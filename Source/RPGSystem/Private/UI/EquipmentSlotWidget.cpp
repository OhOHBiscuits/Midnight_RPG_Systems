//All rights Reserved Midnight Entertainment Studios LLC
#include "UI/EquipmentSlotWidget.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Equipment)
	{
		if (UEquipmentComponent* Found = AutoResolveEquipment())
		{
			SetEquipmentComponent(Found);
		}
	}
	RefreshFromComponent();
}

void UEquipmentSlotWidget::NativeDestruct()
{
	UnbindFromEquipment();
	Super::NativeDestruct();
}

UEquipmentComponent* UEquipmentSlotWidget::AutoResolveEquipment() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UEquipmentComponent* OnPC = PC->FindComponentByClass<UEquipmentComponent>()) return OnPC;
		if (APlayerState* PS = PC->PlayerState)
			if (UEquipmentComponent* OnPS = PS->FindComponentByClass<UEquipmentComponent>()) return OnPS;
		if (APawn* Pawn = PC->GetPawn())
			if (UEquipmentComponent* OnPawn = Pawn->FindComponentByClass<UEquipmentComponent>()) return OnPawn;

		if (UEquipmentComponent* FromPS = UEquipmentHelperLibrary::GetEquipmentPS(PC)) return FromPS;
	}
	return nullptr;
}

void UEquipmentSlotWidget::SetEquipmentComponent(UEquipmentComponent* InEquipment)
{
	if (Equipment == InEquipment)
	{
		RefreshFromComponent();
		return;
	}
	UnbindFromEquipment();
	BindToEquipment(InEquipment);
	RefreshFromComponent();
}

void UEquipmentSlotWidget::SetSlotTagAndRefresh(FGameplayTag NewSlotTag)
{
	SlotTag = NewSlotTag;
	RefreshFromComponent();
}

void UEquipmentSlotWidget::RefreshFromComponent()
{
	UpdateCacheFromEquipment();
}

void UEquipmentSlotWidget::BindToEquipment(UEquipmentComponent* InEquipment)
{
	Equipment = InEquipment;
	if (!IsValid(Equipment)) return;

	Equipment->OnEquipmentChanged.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
	Equipment->OnEquipmentSlotCleared.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
}

void UEquipmentSlotWidget::UnbindFromEquipment()
{
	if (!IsValid(Equipment)) return;

	Equipment->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
	Equipment->OnEquipmentSlotCleared.RemoveDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
	Equipment = nullptr;
}

void UEquipmentSlotWidget::HandleEquipChanged(FGameplayTag ChangedSlot, UItemDataAsset* /*NewData*/)
{
	if (!SlotTag.IsValid()) return;

	if (ChangedSlot.MatchesTagExact(SlotTag))
	{
		UpdateCacheFromEquipment();
	}
}

void UEquipmentSlotWidget::HandleSlotCleared(FGameplayTag ClearedSlot)
{
	if (!SlotTag.IsValid()) return;

	if (ClearedSlot.MatchesTagExact(SlotTag))
	{
		CurrentItemIDTag = FGameplayTag();
		CurrentItemData  = nullptr;
		BroadcastCache();
	}
}

void UEquipmentSlotWidget::UpdateCacheFromEquipment()
{
	if (!IsValid(Equipment) || !SlotTag.IsValid())
	{
		CurrentItemIDTag = FGameplayTag();
		CurrentItemData  = nullptr;
		BroadcastCache();
		return;
	}

	// Pull a snapshot then find my slot to get ItemIDTag
	TArray<FEquippedEntry> Snapshot;
	Equipment->GetAllEquipped(Snapshot);

	CurrentItemIDTag = FGameplayTag();
	for (const FEquippedEntry& E : Snapshot)
	{
		if (E.SlotTag.MatchesTagExact(SlotTag))
		{
			CurrentItemIDTag = E.ItemIDTag;
			break;
		}
	}

	// Resolve data: try component first, then by ItemIDTag (Standalone-safe)
	CurrentItemData = Equipment->GetEquippedItemData(SlotTag);
	if (!CurrentItemData && CurrentItemIDTag.IsValid())
	{
		CurrentItemData = UInventoryHelpers::FindItemDataByTag(this, CurrentItemIDTag);
	}

	BroadcastCache();
}

void UEquipmentSlotWidget::BroadcastCache()
{
	BP_OnSlotItemIDTagUpdated(CurrentItemIDTag);
	BP_OnSlotUpdated(CurrentItemData);
}

// ---------------- UI-only setters (safe) ----------------

void UEquipmentSlotWidget::BP_SetCurrentItemData(UItemDataAsset* InData, bool bFireEvents)
{
	CurrentItemData = InData;
	CurrentItemIDTag = (InData ? InData->ItemIDTag : FGameplayTag());
	if (bFireEvents)
	{
		BroadcastCache();
	}
}

void UEquipmentSlotWidget::BP_SetCurrentItemIDTag(FGameplayTag InTag, bool bResolveData, bool bFireEvents)
{
	CurrentItemIDTag = InTag;

	if (bResolveData && bResolveDataOnTagSet)
	{
		CurrentItemData = InTag.IsValid()
			? UInventoryHelpers::FindItemDataByTag(this, InTag)
			: nullptr;
	}

	if (bFireEvents)
	{
		BroadcastCache();
	}
}
