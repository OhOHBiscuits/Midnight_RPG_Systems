#include "UI/BaseInventoryWidget.h"
#include "Inventory/InventoryComponent.h"

void UBaseInventoryWidget::SetLinkedInventory(UInventoryComponent* Inventory)
{
	// Unbind old delegates
	if (LinkedInventory)
	{
		LinkedInventory->OnInventoryUpdated.RemoveAll(this);
		LinkedInventory->OnInventoryChanged.RemoveAll(this);
	}
	LinkedInventory = Inventory;

	// Bind new ones
	if (LinkedInventory)
	{
		LinkedInventory->OnInventoryUpdated.AddDynamic(this, &UBaseInventoryWidget::OnInventorySlotUpdated);
		LinkedInventory->OnInventoryChanged.AddDynamic(this, &UBaseInventoryWidget::OnInventoryChanged);
		// Initial build
		RebuildInventoryGrid(LinkedInventory);
	}
}

void UBaseInventoryWidget::HandleDragDrop(int32 FromSlot, int32 ToSlot)
{
	if (LinkedInventory)
		LinkedInventory->TryMoveItem(FromSlot, ToSlot); // All logic/networking in InventoryComponent
}

void UBaseInventoryWidget::RequestItemTransfer(
	UInventoryComponent* SourceInventory, int32 SourceSlot,
	UInventoryComponent* TargetInventory, int32 TargetSlot, int32 Quantity)
{
	if (!SourceInventory || !TargetInventory) return;
	if (TargetSlot < 0)
		SourceInventory->TryTransferItem(SourceSlot, TargetInventory);
	else
		SourceInventory->RequestTransferItem(SourceInventory, SourceSlot, TargetInventory, TargetSlot);
}

void UBaseInventoryWidget::OnInventorySlotUpdated(int32 SlotIndex)
{
	OnSlotWidgetUpdate.Broadcast(SlotIndex); // BP/UI can update just this slot
}

void UBaseInventoryWidget::OnInventoryChanged()
{
	if (LinkedInventory)
		RebuildInventoryGrid(LinkedInventory); // Rebuild everything
}

void UBaseInventoryWidget::NativeDestruct()
{
	if (LinkedInventory)
	{
		LinkedInventory->OnInventoryUpdated.RemoveAll(this);
		LinkedInventory->OnInventoryChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}
