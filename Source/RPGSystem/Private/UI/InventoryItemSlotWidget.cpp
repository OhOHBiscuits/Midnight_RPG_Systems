#include "UI/InventoryItemSlotWidget.h"
#include "UI/InventoryPanelWidget.h"
#include "UI/InventoryDragDropOp.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"

#include "Blueprint/WidgetBlueprintLibrary.h"

void UInventoryItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// If the slot was created from BP and ExposeOnSpawn set these,
	// bind now so Construct() already sees a valid InventoryRef.
	if (InventoryRef && SlotIndex >= 0)
	{
		BindToInventory(InventoryRef, SlotIndex);
	}
}

void UInventoryItemSlotWidget::BindToInventory(UInventoryComponent* InInv, int32 InIndex)
{
	if (InventoryRef == InInv && SlotIndex == InIndex) return;

	UnbindFromInventory();

	InventoryRef = InInv;
	SlotIndex = InIndex;

	// Cache parent panel for quick refresh after same-inventory moves
	if (!CachedPanel.IsValid())
	{
		CachedPanel = GetTypedOuter<UInventoryPanelWidget>();
	}

	if (InventoryRef)
	{
		InventoryRef->OnInventoryUpdated.AddDynamic(this, &UInventoryItemSlotWidget::HandleInvSlotUpdated);
		InventoryRef->OnInventoryChanged.AddDynamic(this, &UInventoryItemSlotWidget::HandleInvChanged);
	}

	UpdateFromInventory();
}

void UInventoryItemSlotWidget::UnbindFromInventory()
{
	if (InventoryRef)
	{
		InventoryRef->OnInventoryUpdated.RemoveAll(this);
		InventoryRef->OnInventoryChanged.RemoveAll(this);
	}
	InventoryRef = nullptr;
	SlotIndex = INDEX_NONE;
}

void UInventoryItemSlotWidget::NativeDestruct()
{
	UnbindFromInventory();
	Super::NativeDestruct();
}

void UInventoryItemSlotWidget::UpdateSlotFromInventory()
{
	UpdateFromInventory();
}

void UInventoryItemSlotWidget::HandleInvSlotUpdated(int32 UpdatedIndex)
{
	if (UpdatedIndex == SlotIndex)
	{
		UpdateFromInventory();
	}
}

void UInventoryItemSlotWidget::HandleInvChanged()
{
	// Inventory size or ordering might have changed; refresh if our index is still valid.
	UpdateFromInventory();
}

UItemDataAsset* UInventoryItemSlotWidget::ResolveItemData() const
{
	if (!InventoryRef || SlotIndex == INDEX_NONE) return nullptr;

	const FInventoryItem Item = InventoryRef->GetItem(SlotIndex);
	if (Item.ItemData.IsNull()) return nullptr;

	if (UItemDataAsset* Already = Item.ItemData.Get())
	{
		return Already;
	}
	TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData;
	return Soft.LoadSynchronous(); // safe for occasional UI use
}

void UInventoryItemSlotWidget::UpdateFromInventory()
{
	if (!InventoryRef || SlotIndex == INDEX_NONE)
	{
		SetSlotData(nullptr, INDEX_NONE, nullptr, 0);
		return;
	}

	const FInventoryItem Item = InventoryRef->GetItem(SlotIndex);
	UItemDataAsset* Data = ResolveItemData();
	const int32 Qty = Item.Quantity;

	SetSlotData(InventoryRef, SlotIndex, Data, Qty);
}

bool UInventoryItemSlotWidget::NativeOnDrop(const FGeometry& Geom, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	if (!InventoryRef) return false;

	if (const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op))
	{
		if (!CanAcceptDrop(Drag)) return false;

		bool bSuccess = false;

		if (Drag->SourceInventory == InventoryRef)
		{
			bSuccess = InventoryRef->TryMoveItem(Drag->FromIndex, SlotIndex);

			// Fast local refresh of both slots (inventory events will also fire)
			if (bSuccess && CachedPanel.IsValid())
			{
				CachedPanel->RefreshSlot(SlotIndex);
				CachedPanel->RefreshSlot(Drag->FromIndex);
			}
		}
		else
		{
			// Use component API (handles server routing)
			bSuccess = InventoryRef->RequestTransferItem(Drag->SourceInventory, Drag->FromIndex, InventoryRef, SlotIndex);

			// Fast local refresh of the target slot; source panel refreshes via its own delegate
			if (bSuccess)
			{
				UpdateFromInventory();
				if (CachedPanel.IsValid())
				{
					CachedPanel->RefreshSlot(SlotIndex);
				}
			}
		}

		return bSuccess;
	}
	return false;
}

void UInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry& Geom, const FPointerEvent& Mouse, UDragDropOperation*& OutOperation)
{
	if (!InventoryRef || SlotIndex == INDEX_NONE) return;

	UInventoryDragDropOp* Drag = NewObject<UInventoryDragDropOp>(this);
	Drag->SourceInventory = InventoryRef;
	Drag->FromIndex = SlotIndex;
	Drag->Pivot = EDragPivot::MouseDown;
	OutOperation = Drag;

	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}
