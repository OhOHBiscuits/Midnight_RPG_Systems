// CraftingStationComponent.cpp
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Net/UnrealNetwork.h"

#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"

#include "UObject/UnrealType.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
	// Try to read float "CraftSeconds" from the recipe; default to a tiny value.
	float GetRecipeDurationSeconds(const UCraftingRecipeDataAsset* Recipe, int32 Times)
	{
		if (!Recipe) return 0.01f;
		if (const FProperty* Prop = Recipe->GetClass()->FindPropertyByName(TEXT("CraftSeconds")))
		{
			if (const FFloatProperty* FProp = CastField<FFloatProperty>(Prop))
			{
				const float* Ptr = FProp->ContainerPtrToValuePtr<float>(Recipe);
				if (Ptr) return FMath::Max(0.01f, *Ptr) * FMath::Max(1, Times);
			}
		}
		return 0.01f * FMath::Max(1, Times);
	}

	// Extract (Tag, Amount) pairs from an array property named "Inputs" or "Outputs".
	TArray<TPair<FGameplayTag,int32>> ExtractTagAmountArray(const UObject* Obj, const TCHAR* ArrayPropName)
	{
		TArray<TPair<FGameplayTag,int32>> Pairs;
		if (!Obj) return Pairs;

		const FArrayProperty* ArrProp = FindFProperty<FArrayProperty>(Obj->GetClass(), ArrayPropName);
		if (!ArrProp) return Pairs;

		const void* ContainerPtr = ArrProp->ContainerPtrToValuePtr<void>(Obj);
		FScriptArrayHelper Helper(ArrProp, ContainerPtr);
		const FStructProperty* ElementStructProp = CastField<FStructProperty>(ArrProp->Inner);
		if (!ElementStructProp) return Pairs;

		UScriptStruct* SS = ElementStructProp->Struct;
		if (!SS) return Pairs;

		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			void* ElemPtr = Helper.GetRawPtr(i);

			FGameplayTag FoundTag;
			int32 FoundAmount = 0;

			for (TFieldIterator<FProperty> It(SS); It; ++It)
			{
				if (const FStructProperty* SProp = CastField<FStructProperty>(*It))
				{
					if (SProp->Struct == TBaseStructure<FGameplayTag>::Get())
					{
						const FGameplayTag* TPtr = SProp->ContainerPtrToValuePtr<FGameplayTag>(ElemPtr);
						if (TPtr && TPtr->IsValid())
						{
							FoundTag = *TPtr;
						}
						continue;
					}
				}
				if (const FIntProperty* IProp = CastField<FIntProperty>(*It))
				{
					const FString Name = It->GetName().ToLower();
					if (Name.Contains(TEXT("amount")) || Name.Contains(TEXT("count")) || Name.Contains(TEXT("quantity")))
					{
						if (const int32* V = IProp->ContainerPtrToValuePtr<int32>(ElemPtr))
						{
							FoundAmount = *V;
						}
					}
				}
			}

			if (FoundTag.IsValid() && FoundAmount > 0)
			{
				Pairs.Emplace(FoundTag, FoundAmount);
			}
		}

		return Pairs;
	}

	int32 CountItemsByItemTag(const UInventoryComponent* Inv, const FGameplayTag& ItemTag)
	{
		if (!Inv || !ItemTag.IsValid()) return 0;
		int32 Total = 0;
		const TArray<FInventoryItem>& Items = Inv->GetItems();
		for (const FInventoryItem& It : Items)
		{
			if (!It.IsValid()) continue;
			if (const UItemDataAsset* Data = It.ResolveItemData())
			{
				if (Data->ItemIDTag == ItemTag)
				{
					Total += It.Quantity;
				}
			}
		}
		return Total;
	}

	bool RemoveAndStage(UInventoryComponent* Source, UInventoryComponent* Input, const FGameplayTag& ItemTag, int32 Required)
	{
		if (!Source || !Input || !ItemTag.IsValid() || Required <= 0) return false;

		int32 Remaining = Required;

		if (CountItemsByItemTag(Source, ItemTag) < Required)
		{
			return false;
		}

		const TArray<FInventoryItem>& Items = Source->GetItems();
		for (int32 i = 0; i < Items.Num() && Remaining > 0; ++i)
		{
			const FInventoryItem& It = Items[i];
			if (!It.IsValid()) continue;

			UItemDataAsset* Data = It.ResolveItemData();
			if (!Data || Data->ItemIDTag != ItemTag) continue;

			const int32 Take = FMath::Min(It.Quantity, Remaining);

			if (Source->TryRemoveItem(i, Take))
			{
				Input->TryAddItem(Data, Take);
				Remaining -= Take;
			}
		}

		return Remaining == 0;
	}
}

UCraftingStationComponent::UCraftingStationComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCraftingStationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCraftingStationComponent, ActiveJob);
	DOREPLIFETIME(UCraftingStationComponent, bIsCrafting);
	DOREPLIFETIME(UCraftingStationComponent, bIsPaused);
}

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe, int32 Times)
{
	if (!HasAuth() || bIsCrafting || !Recipe || Times <= 0) return false;
	if (!InputInventory || !OutputInventory) return false;

	if (!MoveRequiredToInput(Recipe, Times, InstigatorActor)) return false;

	ActiveJob = FCraftingJob{};
	ActiveJob.Recipe     = const_cast<UCraftingRecipeDataAsset*>(Recipe);
	ActiveJob.Instigator = InstigatorActor;
	ActiveJob.Count      = Times;
	ActiveJob.StartTime  = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	const float TotalDuration = GetRecipeDurationSeconds(Recipe, Times);
	ActiveJob.EndTime         = ActiveJob.StartTime + TotalDuration;

	bIsCrafting = true;
	bIsPaused   = false;
	OnCraftStarted.Broadcast(ActiveJob);

	GetWorld()->GetTimerManager().SetTimer(
		CraftTimerHandle,
		this, &UCraftingStationComponent::FinishCraft_Internal,
		TotalDuration,
		false
	);

	return true;
}

void UCraftingStationComponent::CancelCraft()
{
	if (!HasAuth() || !bIsCrafting) return;

	GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
	bIsCrafting = false;
	bIsPaused   = false;

	OnCraftFinished.Broadcast(ActiveJob, false);
	ActiveJob = FCraftingJob{};
}

void UCraftingStationComponent::PauseCraft()
{
	if (!HasAuth() || !bIsCrafting || bIsPaused) return;

	bIsPaused = true;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Remaining = FMath::Max(0.f, ActiveJob.EndTime - Now);
	ActiveJob.EndTime = Now + Remaining;

	GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
}

void UCraftingStationComponent::ResumeCraft()
{
	if (!HasAuth() || !bIsCrafting || !bIsPaused) return;

	bIsPaused = false;

	const float Now      = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float TimeLeft = FMath::Max(0.01f, ActiveJob.EndTime - Now);

	GetWorld()->GetTimerManager().SetTimer(
		CraftTimerHandle,
		this, &UCraftingStationComponent::FinishCraft_Internal,
		TimeLeft,
		false
	);
}

void UCraftingStationComponent::FinishCraft_Internal()
{
	const bool bSuccess = (ActiveJob.Recipe != nullptr);

	if (bSuccess)
	{
		DeliverOutputs(ActiveJob.Recipe);
		GiveFinishXPIIfAny(ActiveJob.Recipe, ActiveJob.Instigator.Get(), true);
	}

	OnCraftFinished.Broadcast(ActiveJob, bSuccess);

	bIsCrafting = false;
	bIsPaused   = false;
	ActiveJob   = FCraftingJob{};
}

bool UCraftingStationComponent::MoveRequiredToInput(const UCraftingRecipeDataAsset* Recipe, int32 Times, AActor* InstigatorActor)
{
	if (!HasAuth() || !Recipe || Times <= 0 || !InputInventory) return false;

	UInventoryComponent* Source = UInventoryHelpers::GetInventoryComponent(InstigatorActor ? InstigatorActor : GetOwner());
	if (!Source) return false;

	// Inputs expected to be an array of structs with a GameplayTag + Amount/Count/Quantity int field.
	TArray<TPair<FGameplayTag,int32>> Req = ExtractTagAmountArray(Recipe, TEXT("Inputs"));
	if (Req.Num() == 0) return true;

	for (const auto& Pair : Req)
	{
		const FGameplayTag& Tag = Pair.Key;
		const int32 Needed = Pair.Value * Times;
		if (!RemoveAndStage(Source, InputInventory, Tag, Needed))
		{
			return false;
		}
	}

	return true;
}

void UCraftingStationComponent::DeliverOutputs(const UCraftingRecipeDataAsset* Recipe)
{
	if (!HasAuth() || !Recipe || !OutputInventory) return;

	// Outputs expected to be an array of structs with a GameplayTag + Amount/Count/Quantity int field.
	TArray<TPair<FGameplayTag,int32>> Outs = ExtractTagAmountArray(Recipe, TEXT("Outputs"));
	if (Outs.Num() == 0) return;

	for (const auto& Pair : Outs)
	{
		const FGameplayTag& Tag = Pair.Key;
		const int32 Amount = Pair.Value;

		if (!Tag.IsValid() || Amount <= 0) continue;

		if (UItemDataAsset* Item = UInventoryHelpers::FindItemDataByTag(this, Tag))
		{
			OutputInventory->TryAddItem(Item, Amount);
		}
	}
}

void UCraftingStationComponent::GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* /*Recipe*/, AActor* /*InstigatorActor*/, bool /*bSuccess*/)
{
}

void UCraftingStationComponent::GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const
{
	Out.Reset();
	if (!Viewer) return;

	if (IGameplayTagAssetInterface* GTAI = Cast<IGameplayTagAssetInterface>(Viewer))
	{
		GTAI->GetOwnedGameplayTags(Out);
	}

	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Viewer))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->GetOwnedGameplayTags(Out);
		}
	}

	if (const APawn* Pawn = Cast<APawn>(Viewer))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (IGameplayTagAssetInterface* PSTags = Cast<IGameplayTagAssetInterface>(PS))
			{
				PSTags->GetOwnedGameplayTags(Out);
			}
			if (const IAbilitySystemInterface* ASIPS = Cast<IAbilitySystemInterface>(PS))
			{
				if (UAbilitySystemComponent* ASC = ASIPS->GetAbilitySystemComponent())
				{
					ASC->GetOwnedGameplayTags(Out);
				}
			}
		}

		if (AController* PC = Pawn->GetController())
		{
			if (IGameplayTagAssetInterface* PCTags = Cast<IGameplayTagAssetInterface>(PC))
			{
				PCTags->GetOwnedGameplayTags(Out);
			}
			if (const IAbilitySystemInterface* ASIPC = Cast<IAbilitySystemInterface>(PC))
			{
				if (UAbilitySystemComponent* ASC = ASIPC->GetAbilitySystemComponent())
				{
					ASC->GetOwnedGameplayTags(Out);
				}
			}
		}
	}
}
