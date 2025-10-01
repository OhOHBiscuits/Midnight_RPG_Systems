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

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindToEquipment();
	RefreshVisuals();
}

void UEquipmentSlotWidget::BindToEquipment()
{
	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	CachedEquip = UEquipmentHelperLibrary::GetEquipmentPS(Avatar);

	if (UEquipmentComponent* Equip = CachedEquip.Get())
	{
		// Bind to YOUR delegates:
		//  - FOnEquipmentChanged(FGameplayTag SlotTag, UItemDataAsset* ItemData)
		//  - FOnEquipmentSlotCleared(FGameplayTag SlotTag)
		Equip->OnEquipmentChanged.AddDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
		Equip->OnEquipmentSlotCleared.AddDynamic(this, &UEquipmentSlotWidget::HandleEquipCleared);
	}
}

APlayerState* UEquipmentHelperLibrary::ResolvePlayerState(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;
	if (APlayerState* PS = Cast<APlayerState>(ContextActor)) return PS;
	if (APawn* Pawn = Cast<APawn>(ContextActor)) return Pawn->GetPlayerState();
	if (APlayerController* PC = Cast<APlayerController>(ContextActor)) return PC->PlayerState;
	return ContextActor->GetTypedOuter<APlayerState>(); // last resort
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePlayerState(ContextActor))
		return PS->FindComponentByClass<UEquipmentComponent>();
	return nullptr;
}

UInventoryComponent* UEquipmentHelperLibrary::GetInventoryPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePlayerState(ContextActor))
		return PS->FindComponentByClass<UInventoryComponent>();
	return nullptr;
}

void UEquipmentSlotWidget::HandleEquipChanged(FGameplayTag ChangedSlot, UItemDataAsset* /*NewData*/)
{
	// Only repaint if it's our slot (cheap and tidy)
	if (!SlotTag.IsValid() || ChangedSlot == SlotTag)
	{
		RefreshVisuals();
	}
}

void UEquipmentSlotWidget::HandleEquipCleared(FGameplayTag ClearedSlot)
{
	if (!SlotTag.IsValid() || ClearedSlot == SlotTag)
	{
		RefreshVisuals();
	}
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

	// Optional BP veto first
	if (!CanAcceptDrag_BP(Drag->SourceInventory, Drag->FromIndex))
	{
		OnDropOutcome(false, SlotTag);
		return false;
	}

	APlayerController* PC = GetOwningPlayer();
	AActor* Avatar = PC ? static_cast<AActor*>(PC->GetPawn()) : nullptr;
	if (!Avatar) { OnDropOutcome(false, SlotTag); return false; }

	UEquipmentComponent* Equip = CachedEquip.IsValid() ? CachedEquip.Get() : UEquipmentHelperLibrary::GetEquipmentPS(Avatar);
	if (!Equip) { OnDropOutcome(false, SlotTag); return false; }

	// Resolve the dragged item's data so we can read PreferredEquipSlots
	const FInventoryItem Item = Drag->SourceInventory->GetItem(Drag->FromIndex);
	if (Item.ItemData.IsNull()) { OnDropOutcome(false, SlotTag); return false; }

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data)
	{
		TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData;
		Data = Soft.LoadSynchronous();
	}
	if (!Data) { OnDropOutcome(false, SlotTag); return false; }

	// ------------------------------
	// NEW: filter PreferredEquipSlots by AcceptRootTag (if set)
	TArray<FGameplayTag> Preferred = Data->PreferredEquipSlots;
	if (AcceptRootTag.IsValid())
	{
		TArray<FGameplayTag> Filtered;
		for (const FGameplayTag& T : Preferred)
		{
			if (T.IsValid() && T.MatchesTag(AcceptRootTag))
			{
				Filtered.Add(T);
			}
		}
		Preferred = MoveTemp(Filtered);
	}
	// ------------------------------

	bool bSuccess = false;
	FGameplayTag UsedSlot = SlotTag;

	// Build the order we’ll try to equip into
	TArray<FGameplayTag> TryOrder;
	if (bPreferThisSlotOnDrop)
	{
		// Prefer this visual slot if it belongs to the same root (or no root set)
		if (!AcceptRootTag.IsValid() || SlotTag.MatchesTag(AcceptRootTag))
		{
			TryOrder.Add(SlotTag);
		}

		// Also consider the item’s “secondary” within the same family
		if (Preferred.Num() > 1 && Preferred[1].IsValid() && !TryOrder.Contains(Preferred[1]))
		{
			TryOrder.Add(Preferred[1]);
		}
	}
	else
	{
		// Use asset’s order (already filtered by root), then also this visual slot
		for (const FGameplayTag& T : Preferred) if (T.IsValid()) TryOrder.Add(T);
		if ((!AcceptRootTag.IsValid() || SlotTag.MatchesTag(AcceptRootTag)) && !TryOrder.Contains(SlotTag))
		{
			TryOrder.Add(SlotTag);
		}
	}

	// 1) Empty targets first
	for (const FGameplayTag& T : TryOrder)
	{
		if (Equip->GetEquippedItemData(T) == nullptr)
		{
			UsedSlot = T;
			bSuccess = Equip->TryEquipByInventoryIndex(T, Drag->SourceInventory, Drag->FromIndex);
			break;
		}
	}

	// 2) Swap into THIS visual slot if everything was full and swapping is allowed
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
		if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(Avatar))
		{
			W->TryWieldEquippedInSlot(UsedSlot);
		}
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
	OnRefreshVisualsBP(Data); // let BP set texts, rarity frame, tooltip, etc.

	if (!IconImage) return; // dev wants to drive visuals fully in BP

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
			})
		);
		return;
	}

	IconImage->SetBrushFromTexture(nullptr); // empty slot visual
}

void UEquipmentSlotWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UEquipmentComponent* Equip = UEquipmentHelperLibrary::GetEquipmentPS(GetOwningPlayer()))
	{
		BoundEquip = Equip;
		Equip->OnEquipmentChanged.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleEquipChanged);
		Equip->OnEquipmentSlotCleared.AddUniqueDynamic(this, &UEquipmentSlotWidget::HandleSlotCleared);
	}

	RefreshVisuals(); // initial fill for already equipped items (client-join, etc.)
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

void UEquipmentSlotWidget::HandleSlotCleared(FGameplayTag InSlot)
{
	if (InSlot.MatchesTagExact(SlotTag)) RefreshVisuals();
}


