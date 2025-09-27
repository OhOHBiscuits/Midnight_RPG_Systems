#include "UI/InventoryItemSlotWidget.h"
#include "UI/InventoryPanelWidget.h"
#include "UI/InventoryDragDropOp.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"

UInventoryItemSlotWidget::UInventoryItemSlotWidget(const FObjectInitializer& Obj)
	: Super(Obj)
{
	DragMouseButton = EKeys::LeftMouseButton;
	DragOpClass     = UInventoryDragDropOp::StaticClass();
}

void UInventoryItemSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (InventoryRef && SlotIndex >= 0) { BindToInventory(InventoryRef, SlotIndex); }
}

void UInventoryItemSlotWidget::BindToInventory(UInventoryComponent* InInv, int32 InIndex)
{
	if (InventoryRef == InInv && SlotIndex == InIndex) return;

	UnbindFromInventory();
	InventoryRef = InInv;
	SlotIndex    = InIndex;

	if (!CachedPanel.IsValid()) { CachedPanel = GetTypedOuter<UInventoryPanelWidget>(); }

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
	SlotIndex    = INDEX_NONE;
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
	if (UpdatedIndex == SlotIndex) { UpdateFromInventory(); }
}

void UInventoryItemSlotWidget::HandleInvChanged()
{
	UpdateFromInventory();
}

UItemDataAsset* UInventoryItemSlotWidget::ResolveItemData() const
{
	if (!InventoryRef || SlotIndex == INDEX_NONE) return nullptr;

	const FInventoryItem Item = InventoryRef->GetItem(SlotIndex);
	if (Item.ItemData.IsNull()) return nullptr;

	if (UItemDataAsset* Already = Item.ItemData.Get()) return Already;

	return TSoftObjectPtr<UItemDataAsset>(Item.ItemData).LoadSynchronous();
}

void UInventoryItemSlotWidget::UpdateFromInventory()
{
	if (!InventoryRef || SlotIndex == INDEX_NONE)
	{
		SetSlotData(nullptr, INDEX_NONE, nullptr, 0);
		return;
	}
	const FInventoryItem Item = InventoryRef->GetItem(SlotIndex);
	SetSlotData(InventoryRef, SlotIndex, ResolveItemData(), Item.Quantity);
}

/* ---------- Input & Drag ---------- */

FReply UInventoryItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& MouseEvent)
{
	// Right click → BP hook, no drag
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotRightClick(Geo, MouseEvent);
		return bConsumeRightClick ? FReply::Handled() : FReply::Unhandled();
	}

	FReply Reply = Super::NativeOnMouseButtonDown(Geo, MouseEvent);

	const bool bHasItem =
		(InventoryRef && SlotIndex != INDEX_NONE && InventoryRef->GetItem(SlotIndex).Quantity > 0);

	if ((bHasItem || bAllowDragWhenEmpty) && MouseEvent.GetEffectingButton() == DragMouseButton)
	{
		FEventReply ER = UWidgetBlueprintLibrary::DetectDragIfPressed(MouseEvent, this, DragMouseButton);
		if (ER.NativeReply.IsEventHandled()) { return ER.NativeReply; }
	}
	return Reply;
}

void UInventoryItemSlotWidget::NativeOnDragDetected(
	const FGeometry& Geo, const FPointerEvent& MouseEvent, UDragDropOperation*& OutOp)
{
	if (!InventoryRef || SlotIndex == INDEX_NONE) return;

	// BP can fully craft the operation
	UInventoryDragDropOp* BpOp = nullptr;
	if (BuildDragOperation(BpOp))
	{
		OutOp = BpOp;
		return;
	}

	// Native op
	UClass* OpCls = DragOpClass ? DragOpClass.Get() : UInventoryDragDropOp::StaticClass();
	UInventoryDragDropOp* Drag = NewObject<UInventoryDragDropOp>(this, OpCls);
	Drag->SourceInventory = InventoryRef;
	Drag->FromIndex       = SlotIndex;
	Drag->Pivot           = EDragPivot::MouseDown;
	Drag->SourcePanel     = CachedPanel; // for cross-panel refresh

	if (UUserWidget* Visual = CreateDragVisual())
	{
		Drag->DefaultDragVisual = Visual;
	}

	OutOp = Drag;
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

bool UInventoryItemSlotWidget::NativeOnDrop(
	const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	if (!InventoryRef) return false;

	if (const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op))
	{
		if (!CanAcceptDrop(Drag)) return false;

		const bool bSameInventory = (Drag->SourceInventory == InventoryRef);

		// Fire-and-forget RPC; UI refreshes optimistically
		if (bSameInventory)
		{
			InventoryRef->TryMoveItem(Drag->FromIndex, SlotIndex);

			if (CachedPanel.IsValid())
			{
				if (bFullRefreshAfterDropSameInventory)  CachedPanel->RefreshAll();
				else                                     { CachedPanel->RefreshSlot(SlotIndex); CachedPanel->RefreshSlot(Drag->FromIndex); }
			}
		}
		else
		{
			InventoryRef->RequestTransferItem(Drag->SourceInventory, Drag->FromIndex, InventoryRef, SlotIndex);

			if (CachedPanel.IsValid())       { CachedPanel->RefreshAll(); }
			if (Drag->SourcePanel.IsValid()) { Drag->SourcePanel->RefreshAll(); }
		}

		return true; // handled
	}
	return false;
}




bool UInventoryItemSlotWidget::BuildDragOperation_Implementation(UInventoryDragDropOp*& OutOperation)
{
	OutOperation = nullptr;
	return false; 
}

UUserWidget* UInventoryItemSlotWidget::CreateDragVisual_Implementation()
{
	return DragVisualClass ? CreateWidget<UUserWidget>(GetOwningPlayer(), DragVisualClass) : nullptr;
}
