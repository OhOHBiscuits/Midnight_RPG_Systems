#include "DecaySystem/DecayComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UDecayComponent::UDecayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDecayComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveInventory();
	if (Inventory)
	{
		ApplySettingsFromInventoryType();
	}

	if (GetOwner() && GetOwner()->HasAuthority() && bDecayActive && Inventory)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DecayTimerHandle, this, &UDecayComponent::DecayTimerTick,
			FMath::Max(0.05f, DecayCheckInterval), true
		);
		RefreshDecaySlots();
	}
}

void UDecayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().ClearTimer(DecayTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UDecayComponent::ResolveInventory()
{
	if (Inventory) return;
	AActor* SearchActor = SourceActor ? SourceActor : GetOwner();
	if (SearchActor)
	{
		Inventory = UInventoryHelpers::GetInventoryComponent(SearchActor);
	}
}

void UDecayComponent::ApplySettingsFromInventoryType()
{
	if (!Inventory) return;

	static const FGameplayTag T_PlayerBackpack = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.PlayerBackpack"), false);
	static const FGameplayTag T_StorageChest  = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.StorageChest"),  false);
	static const FGameplayTag T_CoolStorage   = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.CoolStorage"),   false);
	static const FGameplayTag T_ColdStorage   = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.ColdStorage"),   false);
	static const FGameplayTag T_StoreHouse    = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.StoreHouse"),    false);
	static const FGameplayTag T_Compost       = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.Compost"),       false);

	const FGameplayTag& Type = Inventory->InventoryTypeTag;

	if (T_PlayerBackpack.IsValid() && Type.MatchesTagExact(T_PlayerBackpack))       { DecaySpeedMultiplier = 3.0f; EfficiencyRating = 0.5f;  }
	else if (T_StorageChest.IsValid() && Type.MatchesTagExact(T_StorageChest))      { DecaySpeedMultiplier = 1.5f; EfficiencyRating = 1.5f;  }
	else if (T_CoolStorage.IsValid() && Type.MatchesTagExact(T_CoolStorage))        { DecaySpeedMultiplier = 0.5f; EfficiencyRating = 2.0f;  }
	else if (T_ColdStorage.IsValid() && Type.MatchesTagExact(T_ColdStorage))        { DecaySpeedMultiplier = 0.0f; EfficiencyRating = 2.5f;  }
	else if (T_StoreHouse.IsValid() && Type.MatchesTagExact(T_StoreHouse))          { DecaySpeedMultiplier = 1.0f; EfficiencyRating = 1.75f; }
	else if (T_Compost.IsValid() && Type.MatchesTagExact(T_Compost))                { DecaySpeedMultiplier = 5.0f; EfficiencyRating = 3.0f;  }
	else                                                                            { DecaySpeedMultiplier = 1.0f; EfficiencyRating = 1.0f;  }
}


int32 UDecayComponent::CalculateBatchOutput(int32 InputUsed) const
{
	if (InputUsed <= 0) return 0;
	return FMath::Max(1, FMath::FloorToInt(InputUsed * FMath::Max(0.f, EfficiencyRating)));
}

float UDecayComponent::GetItemDecaySeconds(const UItemDataAsset* Asset) const
{
	return Asset ? FMath::Max(0.f, Asset->DecayRate) : 0.f; // uses your current field
}

UItemDataAsset* UDecayComponent::ResolveSoftItem(UItemDataAsset* MaybeLoaded, const TSoftObjectPtr<UItemDataAsset>& Soft)
{
	if (MaybeLoaded) return MaybeLoaded;
	if (Soft.ToSoftObjectPath().IsValid())
	{
		return Soft.LoadSynchronous(); // needed in Standalone/cooked
	}
	return nullptr;
}

UItemDataAsset* UDecayComponent::ResolveItemAsset(const FInventoryItem& SlotItem)
{
	if (SlotItem.ItemData.IsValid()) return SlotItem.ItemData.Get();
	if (SlotItem.ItemData.ToSoftObjectPath().IsValid())
	{
		return SlotItem.ItemData.LoadSynchronous();
	}
	return nullptr;
}

void UDecayComponent::RefreshDecaySlots()
{
	DecaySlots.Empty();
	if (!Inventory) return;

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& Slot = Items[i];
		if (Slot.Quantity <= 0) continue;

		UItemDataAsset* Asset = ResolveItemAsset(Slot);
		if (!Asset || !Asset->bCanDecay) continue;

		const float Seconds = GetItemDecaySeconds(Asset);
		if (Seconds <= 0.f) continue;

		if (!ShouldDecayItem(Slot, Asset)) continue;

		const int32 Batch = FMath::Max(1, InputBatchSize);
		if (Slot.Quantity >= Batch)
		{
			DecaySlots.Emplace(i, Seconds, Seconds, Batch);
		}
	}
}

void UDecayComponent::DecayTimerTick()
{
	if (!bDecayActive || !Inventory || !GetOwner() || !GetOwner()->HasAuthority())
		return;

	RefreshDecaySlots();

	for (FDecaySlot& Slot : DecaySlots)
	{
		if (Slot.SlotIndex < 0) continue;

		const FInventoryItem Current = Inventory->GetItem(Slot.SlotIndex);
		if (Current.Quantity <= 0) continue;

		UItemDataAsset* Asset = ResolveItemAsset(Current);
		if (!Asset || !Asset->bCanDecay) continue;

		OnDecayProgress.Broadcast(Slot.SlotIndex, Slot.DecayTimeRemaining, Slot.TotalDecayTime);

		const float Speed = FMath::Max(0.0f, DecaySpeedMultiplier);
		Slot.DecayTimeRemaining -= (DecayCheckInterval * Speed);

		if (Slot.DecayTimeRemaining > 0.f)
			continue;

		const int32 InputUsed = Slot.BatchSize;
		if (Current.Quantity < InputUsed)
		{
			Slot.DecayTimeRemaining = -1.f;
			continue;
		}

		// Resolve output asset (Standalone-safe)
		UItemDataAsset* DecayResult = ResolveSoftItem(nullptr, Asset->DecaysInto);

		if (DecayResult)
		{
			const int32 OutputQty = CalculateBatchOutput(InputUsed);

			// --- Transactional: ADD FIRST, then remove inputs if it fit ---
			const bool bAdded = Inventory->TryAddItem(DecayResult, OutputQty);
			if (bAdded)
			{
				Inventory->RemoveItem(Slot.SlotIndex, InputUsed);
				OnItemDecayed.Broadcast(Slot.SlotIndex, DecayResult, OutputQty);
			}
			else
			{
				// No space/weight/volume — skip deletion to avoid "items vanish"
				UE_LOG(LogTemp, Warning, TEXT("Decay blocked (no capacity) for '%s' at slot %d"),
					*Asset->GetName(), Slot.SlotIndex);
			}
		}
		else
		{
			// No configured output — do nothing (keeps inputs)
			UE_LOG(LogTemp, Warning, TEXT("Decay output missing/not cooked for '%s' at slot %d"),
				Asset ? *Asset->GetName() : TEXT("NullAsset"), Slot.SlotIndex);
		}

		// Refresh timing for this slot if it can continue
		const FInventoryItem After = Inventory->GetItem(Slot.SlotIndex);
		if (After.Quantity >= Slot.BatchSize)
		{
			const float Seconds = GetItemDecaySeconds(Asset);
			Slot.TotalDecayTime     = Seconds;
			Slot.DecayTimeRemaining = Seconds;
		}
		else
		{
			Slot.DecayTimeRemaining = -1.f;
		}
	}
}
