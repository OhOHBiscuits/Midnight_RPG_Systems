#include "UI/InventoryPanelWidget.h"
#include "UI/InventoryItemSlotWidget.h"
#include "UI/InventoryDragDropOp.h"

#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"

void UInventoryPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SlotContainer && WidgetTree)
	{
		if (UWidget* Found = WidgetTree->FindWidget(FName("SlotContainer")))
		{
			SlotContainer = Cast<UPanelWidget>(Found);
		}
		if (!SlotContainer)
		{
			SlotContainer = Cast<UPanelWidget>(GetRootWidget());
		}
	}

	if (bAutoBindOnConstruct && InventoryRef)
	{
		InitializeWithInventory(InventoryRef);
	}
}

void UInventoryPanelWidget::NativeDestruct()
{
	UnbindInventory();
	Super::NativeDestruct();
}

void UInventoryPanelWidget::InitializeWithInventory(UInventoryComponent* InInventory)
{
	if (InInventory == InventoryRef) return;
	UnbindInventory();
	InventoryRef = InInventory;
	BindInventory(InventoryRef);
	RefreshAll();
}

void UInventoryPanelWidget::BindInventory(UInventoryComponent* InInv)
{
	if (!InInv) return;

	InInv->OnInventoryUpdated.AddDynamic(this, &UInventoryPanelWidget::HandleSlotUpdated);
	InInv->OnInventoryChanged.AddDynamic(this, &UInventoryPanelWidget::HandleInventoryChanged);
	InInv->OnWeightChanged.AddDynamic(this, &UInventoryPanelWidget::HandleWeightChanged);
	InInv->OnVolumeChanged.AddDynamic(this, &UInventoryPanelWidget::HandleVolumeChanged);
}

void UInventoryPanelWidget::UnbindInventory()
{
	if (!InventoryRef) return;
	InventoryRef->OnInventoryUpdated.RemoveAll(this);
	InventoryRef->OnInventoryChanged.RemoveAll(this);
	InventoryRef->OnWeightChanged.RemoveAll(this);
	InventoryRef->OnVolumeChanged.RemoveAll(this);
}

void UInventoryPanelWidget::EnsureSlotWidgets(int32 DesiredCount)
{
	if (!SlotContainer || !SlotWidgetClass) return;

	const int32 Current = SlotContainer->GetChildrenCount();

	// Remove extra
	for (int32 i = Current - 1; i >= DesiredCount; --i)
	{
		SlotContainer->RemoveChildAt(i);
	}

	// Add missing and bind BEFORE adding so Construct sees valid refs
	for (int32 i = Current; i < DesiredCount; ++i)
	{
		UInventoryItemSlotWidget* SlotW = CreateWidget<UInventoryItemSlotWidget>(this, SlotWidgetClass);

		if (InventoryRef)
		{
			SlotW->BindToInventory(InventoryRef, i);
		}

		SlotContainer->AddChild(SlotW);
	}

	// Re-bind existing
	for (int32 i = 0; i < SlotContainer->GetChildrenCount(); ++i)
	{
		if (UInventoryItemSlotWidget* SlotW = Cast<UInventoryItemSlotWidget>(SlotContainer->GetChildAt(i)))
		{
			SlotW->BindToInventory(InventoryRef, i);
		}
	}
}

void UInventoryPanelWidget::QuerySlotData(int32 SlotIndex, UItemDataAsset*& OutData, int32& OutQty) const
{
	OutData = nullptr; 
	OutQty = 0;
	if (!InventoryRef) return;

	const FInventoryItem Item = InventoryRef->GetItem(SlotIndex);
	OutQty = Item.Quantity;

	if (!Item.ItemData.IsNull())
	{
		if (UItemDataAsset* Loaded = Item.ItemData.Get())
		{
			OutData = Loaded;
		}
		else
		{
			TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData;
			OutData = Soft.LoadSynchronous();
		}
	}
}

void UInventoryPanelWidget::RefreshAll()
{
	if (!InventoryRef || !SlotContainer) return;

	OnAboutToRebuild();

	const int32 UILeaves = InventoryRef->GetNumUISlots();
	EnsureSlotWidgets(UILeaves);

	for (int32 i = 0; i < UILeaves; ++i)
	{
		if (UInventoryItemSlotWidget* SlotW = Cast<UInventoryItemSlotWidget>(SlotContainer->GetChildAt(i)))
		{
			UItemDataAsset* Data = nullptr; 
			int32 Qty = 0;
			QuerySlotData(i, Data, Qty);
			SlotW->SetSlotData(InventoryRef, i, Data, Qty);
		}
	}

	OnFinishedRebuild();
}

void UInventoryPanelWidget::RefreshSlot(int32 SlotIndex)
{
	if (!InventoryRef || !SlotContainer) return;
	if (SlotIndex < 0 || SlotIndex >= SlotContainer->GetChildrenCount()) return;

	if (UInventoryItemSlotWidget* SlotW = Cast<UInventoryItemSlotWidget>(SlotContainer->GetChildAt(SlotIndex)))
	{
		SlotW->BindToInventory(InventoryRef, SlotIndex);

		UItemDataAsset* Data = nullptr; 
		int32 Qty = 0;
		QuerySlotData(SlotIndex, Data, Qty);
		SlotW->SetSlotData(InventoryRef, SlotIndex, Data, Qty);
	}
}

void UInventoryPanelWidget::HandleSlotUpdated(int32 SlotIndex)   { RefreshSlot(SlotIndex); }
void UInventoryPanelWidget::HandleInventoryChanged()              { if (!bDeferFullRebuild) RefreshAll(); }
void UInventoryPanelWidget::HandleWeightChanged(float)            { }
void UInventoryPanelWidget::HandleVolumeChanged(float)            { }

/* ---------- Panel background drop (auto-place) ---------- */

bool UInventoryPanelWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	if (!bAcceptDropsOnPanel || !InventoryRef) return Super::NativeOnDrop(Geo, Ev, Op);

	if (TryAutoPlaceDrag(InventoryRef, Op))
	{
		RefreshAll();
		return true;
	}

	return Super::NativeOnDrop(Geo, Ev, Op);
}

bool UInventoryPanelWidget::TryAutoPlaceDrag(UInventoryComponent* TargetInv, UDragDropOperation* Op)
{
	const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op);
	if (!Drag || !TargetInv || !Drag->SourceInventory) return false;

	const bool bSameInventory = (Drag->SourceInventory == TargetInv);
	const int32 Num = TargetInv->GetNumUISlots();

	const FInventoryItem SourceItem = Drag->SourceInventory->GetItem(Drag->FromIndex);
	if (SourceItem.Quantity <= 0) return false;

	auto Attempt = [&](int32 SlotIdx)
	{
		if (bSameInventory) TargetInv->TryMoveItem(Drag->FromIndex, SlotIdx);
		else                TargetInv->RequestTransferItem(Drag->SourceInventory, Drag->FromIndex, TargetInv, SlotIdx);
	};

	// 1) Prefer stacking
	int32 ChosenIndex = INDEX_NONE;
	if (!SourceItem.ItemData.IsNull())
	{
		const FSoftObjectPath SrcPath = SourceItem.ItemData.ToSoftObjectPath();
		for (int32 i = 0; i < Num; ++i)
		{
			const FInventoryItem Tgt = TargetInv->GetItem(i);
			if (Tgt.Quantity > 0 && !Tgt.ItemData.IsNull() && Tgt.ItemData.ToSoftObjectPath() == SrcPath)
			{
				ChosenIndex = i; break;
			}
		}
	}

	// 2) First empty if no stack
	if (ChosenIndex == INDEX_NONE)
	{
		for (int32 i = 0; i < Num; ++i)
		{
			if (TargetInv->GetItem(i).Quantity <= 0) { ChosenIndex = i; break; }
		}
	}

	if (ChosenIndex == INDEX_NONE) return false; // nowhere to place

	// Fire-and-forget the request, then optimistic refresh both views
	Attempt(ChosenIndex);

	RefreshAll();
	if (UInventoryPanelWidget* SrcPanel = Drag->SourcePanel.Get())
	{
		if (SrcPanel != this) { SrcPanel->RefreshAll(); }
	}

	return true;
}
