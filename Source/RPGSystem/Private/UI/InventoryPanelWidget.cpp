#include "UI/InventoryPanelWidget.h"
#include "UI/InventoryItemSlotWidget.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"

void UInventoryPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Fallback: auto-find by name "SlotContainer" if not set explicitly
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

	// Add missing and bind immediately
	for (int32 i = Current; i < DesiredCount; ++i)
	{
		UInventoryItemSlotWidget* SlotW = CreateWidget<UInventoryItemSlotWidget>(this, SlotWidgetClass);
		SlotContainer->AddChild(SlotW);

		if (InventoryRef)
		{
			SlotW->BindToInventory(InventoryRef, i);
		}
	}

	// Re-bind any existing children to ensure they’re connected after size changes
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
		// Ensure bound (safe to repeat)
		SlotW->BindToInventory(InventoryRef, SlotIndex);

		UItemDataAsset* Data = nullptr; 
		int32 Qty = 0;
		QuerySlotData(SlotIndex, Data, Qty);
		SlotW->SetSlotData(InventoryRef, SlotIndex, Data, Qty);
	}
}

void UInventoryPanelWidget::HandleSlotUpdated(int32 SlotIndex)
{
	RefreshSlot(SlotIndex);
}

void UInventoryPanelWidget::HandleInventoryChanged()
{
	if (bDeferFullRebuild) return;
	RefreshAll();
}

void UInventoryPanelWidget::HandleWeightChanged(float /*NewW*/) { }
void UInventoryPanelWidget::HandleVolumeChanged(float /*NewV*/) { }
