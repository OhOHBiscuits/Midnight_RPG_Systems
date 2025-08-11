#include "Crafting/CraftingComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"
#include "FuelSystem/FuelComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

#include "Crafting/CraftingProficiencyInterface.h"
#include "Crafting/CraftingProficiencyComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

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

static UCraftingProficiencyComponent* FindProficiencyComponentOnChain(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (UCraftingProficiencyComponent* C = Actor->FindComponentByClass<UCraftingProficiencyComponent>())
		return C;

	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (UCraftingProficiencyComponent* C2 = Pawn->FindComponentByClass<UCraftingProficiencyComponent>())
			return C2;
		if (AController* Cntrl = Pawn->GetController())
		{
			if (UCraftingProficiencyComponent* C3 = Cntrl->FindComponentByClass<UCraftingProficiencyComponent>())
				return C3;
			if (Cntrl->PlayerState)
				if (UCraftingProficiencyComponent* C4 = Cntrl->PlayerState->FindComponentByClass<UCraftingProficiencyComponent>())
					return C4;
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(Actor))
	{
		if (UCraftingProficiencyComponent* C5 = PC->FindComponentByClass<UCraftingProficiencyComponent>())
			return C5;
		if (PC->PlayerState)
			if (UCraftingProficiencyComponent* C6 = PC->PlayerState->FindComponentByClass<UCraftingProficiencyComponent>())
				return C6;
	}

	if (AActor* Owner = Actor->GetOwner())
		return FindProficiencyComponentOnChain(Owner);

	return nullptr;
}

int32 UCraftingComponent::GetInteractorProficiency(AActor* Interactor, const FGameplayTag& SkillTag) const
{
	if (!Interactor || !SkillTag.IsValid()) return 0;

	if (Interactor->GetClass()->ImplementsInterface(UCraftingProficiencyInterface::StaticClass()))
	{
		return ICraftingProficiencyInterface::Execute_GetCraftingProficiencyLevel(Interactor, SkillTag);
	}

	if (const UCraftingProficiencyComponent* Comp = FindProficiencyComponentOnChain(Interactor))
	{
		return Comp->GetLevel(SkillTag);
	}

	return 0;
}

bool UCraftingComponent::IsProficiencySatisfied(UCraftingRecipeDataAsset* Recipe, AActor* Interactor, int32& OutCurrentLevel) const
{
	OutCurrentLevel = 0;
	if (!Recipe) return false;

	if (!Recipe->RequiredSkillTag.IsValid() || Recipe->RequiredSkillLevel <= 0)
		return true;

	OutCurrentLevel = GetInteractorProficiency(Interactor, Recipe->RequiredSkillTag);
	return OutCurrentLevel >= Recipe->RequiredSkillLevel;
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

	int32 CurLvl = 0;
	if (!IsProficiencySatisfied(Recipe, Interactor, CurLvl))
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

	int32 CurLvl = 0;
	if (!IsProficiencySatisfied(Recipe, Interactor, CurLvl))
	{
		OnCraftingProficiencyFailed.Broadcast(Recipe->RequiredSkillTag, Recipe->RequiredSkillLevel, CurLvl);
		return false;
	}

	if (!CanCraft(Recipe, Interactor)) return false;
	if (!SelectOutputInventory(Interactor)) return false;

	ConsumeInputs(Recipe, Interactor);

	ActiveRecipe = Recipe;
	ActiveRecipeID = Recipe->RecipeID;
	bIsCrafting = true;
	RemainingRepeats = RepeatCount - 1;
	CachedInteractor = Interactor;

	StartCraftTimer(Recipe, CurLvl);
	OnCraftingStarted.Broadcast(ActiveRecipeID);
	return true;
}

void UCraftingComponent::StartCraftTimer(UCraftingRecipeDataAsset* Recipe, int32 InteractorLevel)
{
	const float BaseSeconds = FMath::Max(0.f, Recipe->CraftSeconds);
	float Factor = 1.0f;

	if (Recipe->ProficiencyToTimeFactor)
	{
		Factor = FMath::Max(0.01f, Recipe->ProficiencyToTimeFactor->GetFloatValue((float)InteractorLevel));
	}

	if (CachedInteractor.IsValid() && CachedInteractor->GetClass()->ImplementsInterface(UCraftingProficiencyInterface::StaticClass()))
	{
		const float IfcFactor = ICraftingProficiencyInterface::Execute_GetCraftingTimeFactor(CachedInteractor.Get(), Recipe->RequiredSkillTag);
		if (IfcFactor > 0.f) Factor *= IfcFactor;
	}

	const float Speed = FMath::Max(0.01f, CraftSpeedMultiplier);
	RemainingSeconds = (BaseSeconds * Factor) / Speed;

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

void UCraftingComponent::GrantOutputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor, int32 InteractorLevel)
{
	if (UInventoryComponent* OutInv = SelectOutputInventory(Interactor))
	{
		for (const auto& Out : Recipe->Outputs)
		{
			if (!Out.ItemID.IsValid() || Out.Quantity <= 0) continue;

			int32 Qty = Out.Quantity;
			if (Recipe->ProficiencyToYieldFactor)
			{
				const float YieldFactor = FMath::Max(0.f, Recipe->ProficiencyToYieldFactor->GetFloatValue((float)InteractorLevel));
				Qty = FMath::Max(0, FMath::FloorToInt(Out.Quantity * (YieldFactor <= 0.f ? 1.f : YieldFactor)));
			}

			if (UItemDataAsset* OutAsset = UInventoryHelpers::FindItemDataByTag(this, Out.ItemID))
			{
				if (Qty > 0) OutInv->TryAddItem(OutAsset, Qty);
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
		const int32 CurLvl = CachedInteractor.IsValid()
			? GetInteractorProficiency(CachedInteractor.Get(), ActiveRecipe->RequiredSkillTag)
			: 0;

		GrantOutputs(ActiveRecipe, CachedInteractor.Get(), CurLvl);
		OnCraftingCompleted.Broadcast(ActiveRecipeID);

		if (RemainingRepeats > 0)
		{
			int32 RecheckLvl = 0;
			if (IsProficiencySatisfied(ActiveRecipe, CachedInteractor.Get(), RecheckLvl) &&
				HasRequiredInputs(ActiveRecipe, CachedInteractor.Get()))
			{
				ConsumeInputs(ActiveRecipe, CachedInteractor.Get());
				--RemainingRepeats;
				StartCraftTimer(ActiveRecipe, RecheckLvl);
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
