// Source/RPGSystem/Private/UI/EquipmentSlotWidget.cpp
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

void UEquipmentSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BindToEquipment();
	RefreshVisuals();
}

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshVisuals();
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

void UEquipmentSlotWidget::BindToEquipment()
{
	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;

	BoundEquip = UEquipmentHelperLibrary::GetEquipmentPS(Avatar);
	if (UEquipmentComponent* Equip = BoundEquip.Get())
	{
		Equip->OnEquipmentChanged.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
		Equip->OnEquipmentSlotCleared.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
	}
}

void UEquipmentSlotWidget::HandleEquipChanged(FGameplayTag InSlot, UItemDataAsset* /*Item*/)
{
	if (!SlotTag.IsValid() || InSlot.MatchesTagExact(SlotTag)) RefreshVisuals();
}

void UEquipmentSlotWidget::HandleSlotCleared(FGameplayTag InSlot)
{
	if (!SlotTag.IsValid() || InSlot.MatchesTagExact(SlotTag)) RefreshVisuals();
}

void UEquipmentSlotWidget::NativeOnDragEnter(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	Super::NativeOnDragEnter(Geo, Ev, Op);
	OnDragHoverChanged(true);
}

void UEquipmentSlotWidget::NativeOnDragLeave(const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	Super::NativeOnDragLeave(Ev, Op);
	OnDragHoverChanged(false);
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& Geo, const FDragDropEvent& Ev, UDragDropOperation* Op)
{
	OnDragHoverChanged(false);

	const UInventoryDragDropOp* Drag = Cast<UInventoryDragDropOp>(Op);
	if (!Drag || !Drag->SourceInventory || Drag->FromIndex < 0 || !SlotTag.IsValid())
	{
		OnDropOutcome(false, SlotTag);
		return false;
	}
	if (!CanAcceptDrag_BP(Drag->SourceInventory, Drag->FromIndex))
	{
		OnDropOutcome(false, SlotTag);
		return false;
	}

	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	if (!Avatar) { OnDropOutcome(false, SlotTag); return false; }

	UEquipmentComponent* Equip = BoundEquip.IsValid() ? BoundEquip.Get() : UEquipmentHelperLibrary::GetEquipmentPS(Avatar);
	if (!Equip) { OnDropOutcome(false, SlotTag); return false; }

	// Read the dragged item’s asset (for PreferredEquipSlots)
	const FInventoryItem Item = Drag->SourceInventory->GetItem(Drag->FromIndex);
	if (Item.ItemData.IsNull()) { OnDropOutcome(false, SlotTag); return false; }

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) { TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData; Data = Soft.LoadSynchronous(); }
	if (!Data) { OnDropOutcome(false, SlotTag); return false; }

	// Filter item’s preferences by our family root if set (Weapon vs Armor)
	TArray<FGameplayTag> Preferred = Data->PreferredEquipSlots;
	if (AcceptRootTag.IsValid())
	{
		TArray<FGameplayTag> Filtered;
		for (const FGameplayTag& T : Preferred) if (T.MatchesTag(AcceptRootTag)) Filtered.Add(T);
		Preferred = MoveTemp(Filtered);
	}

	bool bSuccess = false;
	FGameplayTag UsedSlot = SlotTag;

	// Build try order
	TArray<FGameplayTag> TryOrder;
	if (bPreferThisSlotOnDrop && (!AcceptRootTag.IsValid() || SlotTag.MatchesTag(AcceptRootTag)))
	{
		TryOrder.Add(SlotTag);
	}
	for (const FGameplayTag& T : Preferred) if (T.IsValid() && !TryOrder.Contains(T)) TryOrder.Add(T);

	// 1) Empty first
	for (const FGameplayTag& T : TryOrder)
	{
		if (Equip->GetEquippedItemData(T) == nullptr)
		{
			UsedSlot = T;
			bSuccess = Equip->TryEquipByInventoryIndex(T, Drag->SourceInventory, Drag->FromIndex);
			break;
		}
	}

	// 2) Swap into our visual slot if all were full
	if (!bSuccess && bAllowSwapWhenFull)
	{
		if (Equip->TryUnequipSlotToInventory(SlotTag, Drag->SourceInventory))
		{
			UsedSlot = SlotTag;
			bSuccess = Equip->TryEquipByInventoryIndex(SlotTag, Drag->SourceInventory, Drag->FromIndex);
		}
	}

	// Optional auto-wield
	if (bSuccess && bWieldAfterEquip)
	{
		if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar)) W->TryWieldEquippedInSlot(UsedSlot);
	}

	if (bSuccess) RefreshVisuals();
	OnDropOutcome(bSuccess, UsedSlot);
	return bSuccess;
}

void UEquipmentSlotWidget::RefreshVisuals()
{
	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	if (!Avatar) return;

	UItemDataAsset* Data = UEquipmentHelperLibrary::GetEquippedItemData(Avatar, SlotTag);
	OnRefreshVisualsBP(Data);

	if (!IconImage) return;

	if (Data && Data->Icon.IsValid())
	{
		IconImage->SetBrushFromTexture(Data->Icon.Get());
		return;
	}

	if (Data && Data->Icon.ToSoftObjectPath().IsValid())
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
			}));
		return;
	}

	IconImage->SetBrushFromTexture(nullptr);
}
