#include "Crafting/CraftingComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"
#include "FuelSystem/FuelComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCraftingComponent, bIsCrafting);
	DOREPLIFETIME(UCraftingComponent, ActiveRecipeID);
	DOREPLIFETIME(UCraftingComponent, RemainingSeconds);
	DOREPLIFETIME(UCraftingComponent, RemainingRepeats);
}

bool UCraftingComponent::IsFuelSatisfied(UCraftingRecipeDataAsset* Recipe) const
{
	if (!Recipe || !Recipe->bRequiresFuel) return true;
	return FuelComponent && FuelComponent->bIsBurning;
}

void UCraftingComponent::GatherInputSources(AActor* Interactor, TArray<UInventoryComponent*>& OutSources) const
{
	OutSources.Reset();

	if (InputInventory) OutSources.Add(InputInventory);

	if (bIncludeInteractorInventory && Interactor)
	{
		if (UInventoryComponent* InteractorInv = UInventoryHelpers::GetInventoryComponent(Interactor))
		{
			OutSources.AddUnique(InteractorInv);
		}
	}

	if (bUseNearbyPublicInventories)
	{
		const AActor* Owner = GetOwner();
		if (!Owner) return;

		TArray<AActor*> All;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), All);
		const FVector Center = Owner->GetActorLocation();

		for (AActor* A : All)
		{
			if (!A || A == Owner) continue;
			if (FVector::DistSquared(Center, A->GetActorLocation()) > FMath::Square(PublicInventorySearchRadius)) continue;
			if (A->IsA(APawn::StaticClass())) continue;

			if (UInventoryComponent* Inv = A->FindComponentByClass<UInventoryComponent>())
			{
				if (PublicStorageTypeTag.IsValid() && !Inv->InventoryTypeTag.MatchesTagExact(PublicStorageTypeTag))
					continue;
				OutSources.AddUnique(Inv);
			}
		}
	}
}

UInventoryComponent* UCraftingComponent::SelectOutputInventory(AActor* Interactor) const
{
	if (OutputInventory) return OutputInventory;
	if (!bFallbackOutputToPublicStorage) return nullptr;

	const AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), All);
	const FVector Center = Owner->GetActorLocation();

	UInventoryComponent* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* A : All)
	{
		if (!A || A == Owner) continue;
		if (FVector::DistSquared(Center, A->GetActorLocation()) > FMath::Square(PublicInventorySearchRadius)) continue;
		if (A->IsA(APawn::StaticClass())) continue;

		if (UInventoryComponent* Inv = A->FindComponentByClass<UInventoryComponent>())
		{
			if (PublicStorageTypeTag.IsValid() && !Inv->InventoryTypeTag.MatchesTagExact(PublicStorageTypeTag))
				continue;

			const float D = FVector::DistSquared(Center, A->GetActorLocation());
			if (D < BestDistSq)
			{
				Best = Inv;
				BestDistSq = D;
			}
		}
	}
	return Best;
}

int32 UCraftingComponent::CountAcrossSources(const FGameplayTag& ItemID, const TArray<UInventoryComponent*>& Sources) const
{
	int32 Total = 0;
	for (UInventoryComponent* Inv : Sources)
	{
		if (!Inv) continue;
		Total += Inv->GetNumItemsOfType(ItemID);
	}
	return Total;
}

bool UCraftingComponent::RemoveFromSources(const FGameplayTag& ItemID, int32 Quantity, const TArray<UInventoryComponent*>& Sources)
{
	int32 Remaining = Quantity;
	for (UInventoryComponent* Inv : Sources)
	{
		if (!Inv || Remaining <= 0) break;

		while (Remaining > 0)
		{
			int32 Slot = Inv->FindSlotWithItemID(ItemID);
			if (Slot == INDEX_NONE) break;

			const FInventoryItem SlotItem = Inv->GetItem(Slot);
			const int32 Delta = FMath::Min(SlotItem.Quantity, Remaining);
			if (!Inv->RemoveItem(Slot, Delta)) break;

			Remaining -= Delta;
		}
	}
	return Remaining <= 0;
}

bool UCraftingComponent::HasRequiredInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor) const
{
	if (!Recipe) return false;

	TArray<UInventoryComponent*> Sources;
	GatherInputSources(Interactor, Sources);
	if (Sources.Num() == 0) return false;

	for (const auto& Req : Recipe->Inputs)
	{
		if (!Req.ItemID.IsValid() || Req.Quantity <= 0) return false;
		if (CountAcrossSources(Req.ItemID, Sources) < Req.Quantity) return false;
	}
	return true;
}

bool UCraftingComponent::HasOutputSpace(UCraftingRecipeDataAsset* Recipe, AActor* Interactor) const
{
	if (!Recipe) return false;
	return SelectOutputInventory(Interactor) != nullptr;
}

bool UCraftingComponent::CanCraft(UCraftingRecipeDataAsset* Recipe, AActor* Interactor) const
{
	if (!Recipe) return false;
	if (Recipe->StationTypeTag.IsValid() && !StationTypeTag.MatchesTagExact(Recipe->StationTypeTag))
		return false;
	return HasRequiredInputs(Recipe, Interactor) && HasOutputSpace(Recipe, Interactor) && IsFuelSatisfied(Recipe);
}

bool UCraftingComponent::TryStartCraft(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor)
{
	RepeatCount = FMath::Max(1, RepeatCount);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return StartCraft_Internal(Recipe, RepeatCount, Interactor);
	}
	ServerStartCraft(Recipe, RepeatCount, Interactor);
	return true;
}

void UCraftingComponent::TryCancelCraft()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClearCraftState(true);
	}
	else
	{
		ServerCancelCraft();
	}
}

bool UCraftingComponent::StartCraft_Internal(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor)
{
	if (!Recipe || bIsCrafting) return false;
	if (!CanCraft(Recipe, Interactor)) return false;
	if (!SelectOutputInventory(Interactor)) return false;

	ConsumeInputs(Recipe, Interactor);

	ActiveRecipe = Recipe;
	ActiveRecipeID = Recipe->RecipeID;
	bIsCrafting = true;
	RemainingRepeats = RepeatCount - 1;
	CachedInteractor = Interactor;

	StartCraftTimer(Recipe);
	OnCraftingStarted.Broadcast(ActiveRecipeID);
	return true;
}

void UCraftingComponent::StartCraftTimer(UCraftingRecipeDataAsset* Recipe)
{
	const float Speed = FMath::Max(0.01f, CraftSpeedMultiplier);
	RemainingSeconds = FMath::Max(0.f, Recipe->CraftSeconds) / Speed;
	GetWorld()->GetTimerManager().SetTimer(CraftTimer, this, &UCraftingComponent::CraftTick, 1.0f, true);
}

void UCraftingComponent::ConsumeInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor)
{
	TArray<UInventoryComponent*> Sources;
	GatherInputSources(Interactor, Sources);

	for (const auto& Req : Recipe->Inputs)
	{
		if (!Req.ItemID.IsValid() || Req.Quantity <= 0) continue;
		RemoveFromSources(Req.ItemID, Req.Quantity, Sources);
	}
}

void UCraftingComponent::GrantOutputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor)
{
	if (UInventoryComponent* OutInv = SelectOutputInventory(Interactor))
	{
		for (const auto& Out : Recipe->Outputs)
		{
			if (!Out.ItemID.IsValid() || Out.Quantity <= 0) continue;
			if (UItemDataAsset* OutAsset = UInventoryHelpers::FindItemDataByTag(this, Out.ItemID))
			{
				OutInv->TryAddItem(OutAsset, Out.Quantity);
			}
		}
	}
}

void UCraftingComponent::CraftTick()
{
	if (!bIsCrafting || !ActiveRecipe)
	{
		GetWorld()->GetTimerManager().ClearTimer(CraftTimer);
		return;
	}

	if (!IsFuelSatisfied(ActiveRecipe))
	{
		ClearCraftState(true);
		return;
	}

	RemainingSeconds = FMath::Max(0.f, RemainingSeconds - 1.f);
	OnCraftingProgress.Broadcast(ActiveRecipeID, RemainingSeconds);

	if (RemainingSeconds <= 0.f)
	{
		GrantOutputs(ActiveRecipe, CachedInteractor.Get());
		OnCraftingCompleted.Broadcast(ActiveRecipeID);

		if (RemainingRepeats > 0)
		{
			if (CanCraft(ActiveRecipe, CachedInteractor.Get()))
			{
				ConsumeInputs(ActiveRecipe, CachedInteractor.Get());
				--RemainingRepeats;
				StartCraftTimer(ActiveRecipe);
			}
			else
			{
				ClearCraftState(false);
			}
		}
		else
		{
			ClearCraftState(false);
		}
	}
}

void UCraftingComponent::ClearCraftState(bool bBroadcastCancel)
{
	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);

	if (bIsCrafting && bBroadcastCancel)
	{
		OnCraftingCancelled.Broadcast(ActiveRecipeID);
	}

	bIsCrafting = false;
	ActiveRecipe = nullptr;
	ActiveRecipeID = FGameplayTag();
	RemainingSeconds = 0.f;
	RemainingRepeats = 0;
	CachedInteractor.Reset();
}

void UCraftingComponent::ServerStartCraft_Implementation(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor)
{
	StartCraft_Internal(Recipe, RepeatCount, Interactor);
}
bool UCraftingComponent::ServerStartCraft_Validate(UCraftingRecipeDataAsset* /*Recipe*/, int32 /*RepeatCount*/, AActor* /*Interactor*/) { return true; }

void UCraftingComponent::ServerCancelCraft_Implementation()
{
	ClearCraftState(true);
}
bool UCraftingComponent::ServerCancelCraft_Validate() { return true; }
