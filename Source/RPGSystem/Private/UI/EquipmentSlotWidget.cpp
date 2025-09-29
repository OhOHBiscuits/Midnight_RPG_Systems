#include "UI/EquipmentSlotWidget.h"
#include "UI/InventoryDragDropOp.h"

#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/Image.h"

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initial paint
	RefreshVisuals();

	// If your EquipmentComponent broadcasts OnEquipmentSlotChanged(FGameplayTag),
	// you can bind here to auto-refresh when replication updates:
	if (APlayerController* PC = GetOwningPlayer())
	{
		AActor* Avatar = PC->GetPawn();
		if (UEquipmentComponent* Equip = UEquipmentHelperLibrary::GetEquipmentPS(Avatar))
		{
			// Pseudocode: uncomment if you added the event.
			// Equip->OnEquipmentSlotChanged.AddDynamic(this, &UEquipmentSlotWidget::RefreshVisuals);
		}
	}
}

// Drag from inventory onto this slot → smart-equip:
// 1) If THIS slot is empty → equip here.
// 2) Else, if the item advertises a secondary and it’s empty → equip there.
// 3) Else, if swapping is allowed → move the current item in THIS slot back to the source inventory, then equip here.
bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op);
	if (!Drag || !Drag->SourceInventory || Drag->FromIndex < 0 || !SlotTag.IsValid())
	{
		return false; // Not our payload or misconfigured widget
	}

	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	if (!Avatar) return false;

	UEquipmentComponent* Equip = UEquipmentHelperLibrary::GetEquipmentPS(Avatar);
	if (!Equip) return false;

	// We need the dragged item’s data to read PreferredEquipSlots
	const FInventoryItem Item = Drag->SourceInventory->GetItem(Drag->FromIndex);
	if (Item.ItemData.IsNull()) return false;

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data)
	{
		TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData;
		Data = Soft.LoadSynchronous(); // UI-side & tiny; fine here
	}
	if (!Data) return false;

	// 1) Try THIS slot if it's free
	const bool bThisSlotEmpty = (Equip->GetEquippedItemData(SlotTag) == nullptr);
	if (bThisSlotEmpty)
	{
		const bool bReq = Equip->TryEquipByInventoryIndex(SlotTag, Drag->SourceInventory, Drag->FromIndex);
		if (bReq && bWieldAfterEquip)
		{
			if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar))
			{
				W->TryWieldEquippedInSlot(SlotTag);
			}
		}
		if (bReq) RefreshVisuals();
		return bReq;
	}

	// 2) Prefer the item’s secondary if defined and empty
	if (Data->PreferredEquipSlots.Num() > 1)
	{
		const FGameplayTag Secondary = Data->PreferredEquipSlots[1];
		if (Secondary.IsValid() && Equip->GetEquippedItemData(Secondary) == nullptr)
		{
			const bool bReq = Equip->TryEquipByInventoryIndex(Secondary, Drag->SourceInventory, Drag->FromIndex);
			if (bReq && bWieldAfterEquip)
			{
				if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar))
				{
					W->TryWieldEquippedInSlot(Secondary);
				}
			}
			if (bReq) RefreshVisuals();
			return bReq;
		}
	}

	// 3) Both full → swap into THIS slot if allowed
	if (bAllowSwapWhenFull)
	{
		const bool bUnequipped = Equip->TryUnequipSlotToInventory(SlotTag, Drag->SourceInventory);
		if (!bUnequipped) return false;

		const bool bEquipped = Equip->TryEquipByInventoryIndex(SlotTag, Drag->SourceInventory, Drag->FromIndex);
		if (bEquipped && bWieldAfterEquip)
		{
			if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar))
			{
				W->TryWieldEquippedInSlot(SlotTag);
			}
		}
		if (bEquipped) RefreshVisuals();
		return bEquipped;
	}

	// Not handling a swap? Then this drop isn’t accepted.
	return false;
}

void UEquipmentSlotWidget::RefreshVisuals()
{
	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	if (!Avatar) return;

	if (UItemDataAsset* Data = UEquipmentHelperLibrary::GetEquippedItemData(Avatar, SlotTag))
	{
		// Only load/show the icon. Keep UI feather-light.
		if (IconImage && Data->Icon.IsValid())
		{
			IconImage->SetBrushFromTexture(Data->Icon.Get());
		}
		else if (IconImage && Data->Icon.ToSoftObjectPath().IsValid())
		{
			FStreamableManager& SM = UAssetManager::GetStreamableManager();
			const FSoftObjectPath Path = Data->Icon.ToSoftObjectPath();

			SM.RequestAsyncLoad(Path,
				FStreamableDelegate::CreateWeakLambda(this, [this, Path]()
				{
					if (!IconImage) return;
					if (UTexture2D* Tex = Cast<UTexture2D>(Path.ResolveObject()))
					{
						IconImage->SetBrushFromTexture(Tex);
					}
				})
			);
		}
	}
	else
	{
		// Empty slot visuals
		if (IconImage) { IconImage->SetBrushFromTexture(nullptr); }
	}
}
