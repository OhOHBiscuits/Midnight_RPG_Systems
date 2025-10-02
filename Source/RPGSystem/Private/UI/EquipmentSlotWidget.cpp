#include "UI/EquipmentSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/Image.h"
#include "GameFramework/PlayerState.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "UI/InventoryDragDropOp.h"
#include "Inventory/InventoryComponent.h"



void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bAutoBindEquipment)
	{
		if (APlayerState* PS = GetOwningPlayerState())
		{
			if (UEquipmentComponent* Equip = PS->FindComponentByClass<UEquipmentComponent>())
			{
				BindToEquipment(Equip);
			}
		}
	}
}

void UEquipmentSlotWidget::NativeDestruct()
{
	if (UEquipmentComponent* Equip = BoundEquip.Get())
	{
		Equip->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
		Equip->OnEquipmentSlotCleared.RemoveDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
	}
	Super::NativeDestruct();
}

void UEquipmentSlotWidget::BindToEquipment(UEquipmentComponent* Equip)
{
	if (!Equip) return;

	if (UEquipmentComponent* Prev = BoundEquip.Get())
	{
		Prev->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
		Prev->OnEquipmentSlotCleared.RemoveDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
	}

	BoundEquip = Equip;
	Equip->OnEquipmentChanged.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
	Equip->OnEquipmentSlotCleared.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);

	RefreshVisuals();
}

void UEquipmentSlotWidget::HandleEquipChanged(FGameplayTag InSlot, UItemDataAsset* InData)
{
	if (InSlot == SlotTag)
	{
		ApplyIcon(InData);
	}
}

void UEquipmentSlotWidget::HandleSlotCleared(FGameplayTag InSlot)
{
	if (InSlot == SlotTag)
	{
		ApplyIcon(nullptr);
	}
}

void UEquipmentSlotWidget::ApplyIcon(UItemDataAsset* Data)
{
	if (!IconImage) return;

	if (Data && Data->Icon.IsValid())
	{
		IconImage->SetBrushFromTexture(Data->Icon.Get());
	}
	else if (Data && Data->Icon.ToSoftObjectPath().IsValid())
	{
		TWeakObjectPtr<UImage> Img = IconImage;
		FStreamableManager& Streamer = UAssetManager::GetStreamableManager();
		Streamer.RequestAsyncLoad(Data->Icon.ToSoftObjectPath(), [Img, Data]()
		{
			if (Img.IsValid())
			{
				if (UTexture2D* Tex = Data->Icon.Get())
				{
					Img->SetBrushFromTexture(Tex);
				}
			}
		});
	}
	else
	{
		IconImage->SetBrushFromTexture(nullptr);
	}
}

void UEquipmentSlotWidget::RefreshVisuals()
{
	UItemDataAsset* Data = UEquipmentHelperLibrary::GetEquippedItemData(GetOwningPlayerState(), SlotTag);
	ApplyIcon(Data);
}

// ---------------- Drag & Drop ----------------
FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Evt)
{
	if (Evt.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(Evt, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(Geo, Evt);
}

void UEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& Geo, const FPointerEvent& Evt, UDragDropOperation*& OutOp)
{
	// Allow BP override to craft custom op
	UInventoryDragDropOp* Op = nullptr;
	if (BuildDragOperation(Op))
	{
		OutOp = Op;
		return;
	}

	// Minimal default operation (no payload required to drop back on equipment)
	Op = NewObject<UInventoryDragDropOp>(this);
	Op->Pivot = EDragPivot::MouseDown;
	OutOp = Op;
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& DD, UDragDropOperation* Op)
{
	const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op);
	if (!Drag) return false;

	// 1) If payload is inventory item -> try equip to this slot
	if (Drag->SourceInventory && Drag->FromIndex >= 0)
	{
		// Let helper pick best or enforce this slot
		return UEquipmentHelperLibrary::EquipBestFromInventoryIndex(
			GetOwningPlayerState(),
			Drag->SourceInventory,
			Drag->FromIndex,
			/*PreferredSlot*/ SlotTag,
			/*bAlsoWield*/ true
		);
	}

	// 2) If payload is another equipment slot op, you could swap here (optional)
	return false;
}
