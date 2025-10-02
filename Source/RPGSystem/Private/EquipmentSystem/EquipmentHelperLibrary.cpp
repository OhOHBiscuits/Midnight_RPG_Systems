#include "EquipmentSystem/EquipmentHelperLibrary.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "Inventory/ItemDataAsset.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryAssetManager.h"

static APlayerState* ResolvePS(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;

	if (const APawn* Pawn = Cast<APawn>(ContextActor))
	{
		return Pawn->GetPlayerState();
	}
	return Cast<APlayerState>(ContextActor);
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePS(ContextActor))
	{
		return PS->FindComponentByClass<UEquipmentComponent>();
	}
	return nullptr;
}

UInventoryComponent* UEquipmentHelperLibrary::GetInventoryPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePS(ContextActor))
	{
		return PS->FindComponentByClass<UInventoryComponent>();
	}
	return nullptr;
}

UWieldComponent* UEquipmentHelperLibrary::GetWieldPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePS(ContextActor))
	{
		return PS->FindComponentByClass<UWieldComponent>();
	}
	return nullptr;
}

bool UEquipmentHelperLibrary::EquipByItemIDTag(AActor* ContextActor, const FGameplayTag& SlotTag, const FGameplayTag& ItemIDTag, bool bAlsoWield)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->TryEquipByItemIDTag(SlotTag, ItemIDTag, bAlsoWield);
	}
	return false;
}

bool UEquipmentHelperLibrary::UnequipSlotToInventory(AActor* ContextActor, const FGameplayTag& SlotTag, UInventoryComponent* DestInventory)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->TryUnequipSlotToInventory(SlotTag, DestInventory);
	}
	return false;
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, const FGameplayTag& PreferredSlot, bool bAlsoWield)
{
	if (!SourceInventory) return false;

	UEquipmentComponent* Equip = GetEquipmentPS(ContextActor);
	if (!Equip) return false;

	// If caller specified a preferred slot, try that first.
	if (PreferredSlot.IsValid())
	{
		if (Equip->TryEquipByInventoryIndex(PreferredSlot, SourceInventory, SourceIndex, bAlsoWield))
		{
			return true;
		}
	}

	// Otherwise, iterate all slots and take the first that accepts the item.
	UItemDataAsset* SourceData = nullptr;
	{
		FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
		SourceData = Item.ItemData.LoadSynchronous();
	}
	if (!SourceData) return false;

	TArray<FGameplayTag> AllSlots;
	Equip->GetAllSlotTags(AllSlots);

	for (const FGameplayTag& S : AllSlots)
	{
		if (Equip->TryEquipByInventoryIndex(S, SourceInventory, SourceIndex, bAlsoWield))
		{
			return true;
		}
	}
	return false;
}

UItemDataAsset* UEquipmentHelperLibrary::GetEquippedItemData(AActor* ContextActor, const FGameplayTag& SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->GetEquippedItemDataForSlot(SlotTag);
	}
	return nullptr;
}
