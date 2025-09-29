// EquipmentComponent.cpp

#include "EquipmentSystem/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryAssetManager.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Actor.h"
#include "UI/EquipmentSlotWidget.h"
#include "Net/UnrealNetwork.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentComponent, Equipped);
}

const FEquipmentSlotDef* UEquipmentComponent::FindSlotDef(FGameplayTag SlotTag) const
{
	for (const FEquipmentSlotDef& S : Slots)
	{
		if (S.SlotTag == SlotTag) return &S;
	}
	return nullptr;
}

int32 UEquipmentComponent::FindEquippedIndex(FGameplayTag SlotTag) const
{
	for (int32 i=0;i<Equipped.Num();++i) if (Equipped[i].SlotTag == SlotTag) return i;
	return INDEX_NONE;
}

bool UEquipmentComponent::IsItemAllowedInSlot(const FEquipmentSlotDef& SlotDef, UItemDataAsset* ItemData) const
{
	if (!ItemData) return false;
	FGameplayTagContainer Owned;
	Owned.AddTag(ItemData->ItemIDTag);
	Owned.AddTag(ItemData->ItemType);
	Owned.AddTag(ItemData->ItemCategory);
	Owned.AddTag(ItemData->ItemSubCategory);
	return SlotDef.AllowedItemQuery.IsEmpty() ? true : SlotDef.AllowedItemQuery.Matches(Owned);
}

void UEquipmentComponent::OnRep_Equipped()
{
	for (const FEquippedEntry& E : Equipped)
	{
		if (!E.SlotTag.IsValid()) continue;
		if (E.ItemIDTag.IsValid())
		{
			UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(E.ItemIDTag, /*bSync*/true);
			OnEquippedItemChanged.Broadcast(E.SlotTag, Data);
		}
		else
		{
			OnEquippedSlotCleared.Broadcast(E.SlotTag);
		}
	}
}

bool UEquipmentComponent::IsEquipped(FGameplayTag SlotTag) const
{
	const int32 Idx = FindEquippedIndex(SlotTag);
	return Idx != INDEX_NONE && Equipped[Idx].ItemIDTag.IsValid();
}

UItemDataAsset* UEquipmentComponent::GetEquippedItemData(FGameplayTag SlotTag) const
{
	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE) return nullptr;
	const FEquippedEntry& E = Equipped[Idx];
	if (!E.ItemIDTag.IsValid()) return nullptr;
	return UInventoryAssetManager::Get().LoadItemDataByTag(E.ItemIDTag, /*bSync*/true);
}

bool UEquipmentComponent::EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex)
{
	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority()) return TryEquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
	}

	if (!FromInventory || !FromInventory->GetItems().IsValidIndex(FromIndex)) return false;
	FInventoryItem Item = FromInventory->GetItem(FromIndex);
	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) return false;

	const FEquipmentSlotDef* SlotDef = FindSlotDef(SlotTag);
	if (!SlotDef) return false;
	if (!IsItemAllowedInSlot(*SlotDef, Data)) return false;

	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE)
	{
		FEquippedEntry NewE; NewE.SlotTag = SlotTag; NewE.ItemIDTag = Data->ItemIDTag;
		Equipped.Add(NewE);
	}
	else
	{
		Equipped[Idx].ItemIDTag = Data->ItemIDTag;
	}
	if (bAutoApplyItemModifiers) ApplyItemModifiers(Data, true);
	OnEquippedItemChanged.Broadcast(SlotTag, Data);
	return true;
}

bool UEquipmentComponent::EquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority()) return TryEquipByItemIDTag(SlotTag, ItemIDTag);
	}

	if (!SlotTag.IsValid() || !ItemIDTag.IsValid()) return false;
	const FEquipmentSlotDef* SlotDef = FindSlotDef(SlotTag);
	if (!SlotDef) return false;

	UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, /*bSync*/true);
	if (!Data) return false;
	if (!IsItemAllowedInSlot(*SlotDef, Data)) return false;

	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE)
	{
		FEquippedEntry NewE; NewE.SlotTag = SlotTag; NewE.ItemIDTag = ItemIDTag;
		Equipped.Add(NewE);
	}
	else
	{
		Equipped[Idx].ItemIDTag = ItemIDTag;
	}
	if (bAutoApplyItemModifiers) ApplyItemModifiers(Data, true);
	OnEquippedItemChanged.Broadcast(SlotTag, Data);
	return true;
}

bool UEquipmentComponent::UnequipSlot(FGameplayTag SlotTag)
{
	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority()) return TryUnequipSlot(SlotTag);
	}

	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE) return false;

	UItemDataAsset* Prev = nullptr;
	if (Equipped[Idx].ItemIDTag.IsValid())
	{
		Prev = UInventoryAssetManager::Get().LoadItemDataByTag(Equipped[Idx].ItemIDTag, true);
	}
	Equipped[Idx].ItemIDTag = FGameplayTag();
	if (bAutoApplyItemModifiers && Prev) ApplyItemModifiers(Prev, false);
	OnEquippedSlotCleared.Broadcast(SlotTag);
	return true;
}

// Client wrappers
bool UEquipmentComponent::TryEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex)
{
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) return EquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
		ServerEquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
		return true;
	}
	return false;
}

bool UEquipmentComponent::TryEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) return EquipByItemIDTag(SlotTag, ItemIDTag);
		ServerEquipByItemIDTag(SlotTag, ItemIDTag);
		return true;
	}
	return false;
}

bool UEquipmentComponent::TryUnequipSlot(FGameplayTag SlotTag)
{
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) return UnequipSlot(SlotTag);
		ServerUnequipSlot(SlotTag);
		return true;
	}
	return false;
}

// RPCs
bool UEquipmentComponent::ServerEquipByInventoryIndex_Validate(FGameplayTag, UInventoryComponent*, int32){ return true; }
void UEquipmentComponent::ServerEquipByInventoryIndex_Implementation(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex)
{
	EquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
}

bool UEquipmentComponent::ServerEquipByItemIDTag_Validate(FGameplayTag, FGameplayTag){ return true; }
void UEquipmentComponent::ServerEquipByItemIDTag_Implementation(FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	EquipByItemIDTag(SlotTag, ItemIDTag);
}

bool UEquipmentComponent::ServerUnequipSlot_Validate(FGameplayTag){ return true; }
void UEquipmentComponent::ServerUnequipSlot_Implementation(FGameplayTag SlotTag)
{
	UnequipSlot(SlotTag);
}

void UEquipmentComponent::ApplyItemModifiers_Implementation(UItemDataAsset* /*ItemData*/, bool /*bApply*/) {}

bool UEquipmentComponent::TryUnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInv)
{
	// If there's no inventory to catch it, just clear the slot. Nothing fancy.
	if (!DestInv)
	{
		return TryUnequipSlot(SlotTag);
	}

	AActor* OwnerActor = GetOwner();
	AController* Ctrl = OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

	// Clients ask the server to do the real work.
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		Server_UnequipSlotToInventory(SlotTag, DestInv, Ctrl);
		return true; // UI can optimistically refresh; replication will reconcile
	}

	// Authority path:
	UItemDataAsset* Data = GetEquippedItemData(SlotTag);
	if (!Data)
	{
		return false; // nothing to unequip
	}

	// Politely put the item back into the target inventory. We add by ItemIDTag (stack-first).
	int32 AddedIndex = INDEX_NONE;
	const bool bAdded = UInventoryHelpers::TryAddByItemIDTag(DestInv, Data->ItemIDTag, /*Qty*/1, AddedIndex);
	if (!bAdded)
	{
		return false; // nowhere to put it
	}

	// Now that it safely lives in an inventory, clear the equipment slot.
	return TryUnequipSlot(SlotTag);
}

void UEquipmentComponent::Server_UnequipSlotToInventory_Implementation(FGameplayTag SlotTag, UInventoryComponent* DestInventory, AController* Requestor)
{
	TryUnequipSlotToInventory(SlotTag, DestInventory);
}

