// Source/RPGSystem/Private/UI/EquipmentSlotWidget.cpp
#include "UI/EquipmentSlotWidget.h"
#include "UI/InventoryDragDropOp.h"

#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"

// Helper: ask equipment if a slot is busy (without adding new API)
static bool SlotHasItem(UEquipmentComponent* Equip, const FGameplayTag& Slot)
{
	return (Equip && Equip->GetEquippedItemData(Slot) != nullptr);
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op);
	if (!Drag || !Drag->SourceInventory || Drag->FromIndex < 0 || !SlotTag.IsValid())
	{
		return false; // Not our kind of payload
	}

	AActor* Avatar = GetOwningPlayerPawn();
	UEquipmentComponent* Equip = UEquipmentHelperLibrary::GetEquipmentPS(Avatar);
	if (!Equip) return false;

	// Resolve the dragged item's data (we need PreferredEquipSlots to consider a Secondary)
	const FInventoryItem Item = Drag->SourceInventory->GetItem(Drag->FromIndex);
	if (Item.ItemData.IsNull()) return false;

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) { TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData; Data = Soft.LoadSynchronous(); }
	if (!Data) return false;

	// Step 1: try to equip directly into THIS slot if it’s free
	if (!SlotHasItem(Equip, SlotTag))
	{
		const bool bReq = Equip->TryEquipByInventoryIndex(SlotTag, Drag->SourceInventory, Drag->FromIndex);
		if (bReq && bWieldAfterEquip)
		{
			if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar)) { W->TryWieldEquippedInSlot(SlotTag); }
		}
		return bReq;
	}

	// Step 2: this slot is full. If the item exposes a Secondary, try that if it’s empty.
	// We look for a second PreferredEquipSlots entry and use it if free.
	if (Data->PreferredEquipSlots.Num() > 1)
	{
		const FGameplayTag Secondary = Data->PreferredEquipSlots[1];
		if (Secondary.IsValid() && !SlotHasItem(Equip, Secondary))
		{
			const bool bReq = Equip->TryEquipByInventoryIndex(Secondary, Drag->SourceInventory, Drag->FromIndex);
			if (bReq && bWieldAfterEquip)
			{
				if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar)) { W->TryWieldEquippedInSlot(Secondary); }
			}
			return bReq;
		}
	}

	// Step 3: both are full. If allowed, swap the item in THIS slot back to the source inventory, then equip here.
	if (bAllowSwapWhenFull)
	{
		const bool bUnequipped = Equip->TryUnequipSlotToInventory(SlotTag, Drag->SourceInventory);
		if (!bUnequipped) return false;

		const bool bEquipped = Equip->TryEquipByInventoryIndex(SlotTag, Drag->SourceInventory, Drag->FromIndex);
		if (bEquipped && bWieldAfterEquip)
		{
			if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar)) { W->TryWieldEquippedInSlot(SlotTag); }
		}
		return bEquipped;
	}

	// Not swapping? Then we don’t handle the drop.
	return false;
}
