// Copyright ...
#include "EquipmentSystem/EquipmentComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/PlayerState.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"

#include "EquipmentSystem/WieldComponent.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentComponent, Equipped);
}

void UEquipmentComponent::OnRep_Equipped()
{
	// Notify UI for all slots (cheap; you can granularize later if desired)
	for (const FEquippedEntry& E : Equipped)
	{
		UItemDataAsset* Data = LoadItemDataByID(E.ItemIDTag);
		OnEquipmentChanged.Broadcast(E.SlotTag, Data);
		OnEquippedItemChanged.Broadcast(E.SlotTag, Data);
		if (!E.ItemIDTag.IsValid())
		{
			OnEquipmentSlotCleared.Broadcast(E.SlotTag);
			OnEquippedSlotCleared.Broadcast(E.SlotTag);
		}
	}
}

int32 UEquipmentComponent::FindEquippedIndex(const FGameplayTag& Slot) const
{
	for (int32 i=0;i<Equipped.Num();++i)
	{
		if (Equipped[i].SlotTag == Slot) return i;
	}
	return INDEX_NONE;
}

bool UEquipmentComponent::IsItemAllowedInSlot(const UItemDataAsset* ItemData, const FGameplayTag& SlotTag) const
{
	if (!ItemData) return false;

	auto Matches = [&](const TArray<FEquipmentSlotDef>& Defs)->bool
	{
		for (const FEquipmentSlotDef& Def : Defs)
		{
			if (Def.SlotTag == SlotTag)
			{
				FGameplayTagContainer ItemTags;

				// If the DataAsset exposes tags via the interface, pull them
				if (const IGameplayTagAssetInterface* TagSrc = Cast<IGameplayTagAssetInterface>(ItemData))
				{
					TagSrc->GetOwnedGameplayTags(ItemTags);
				}

				// Always include the ItemIDTag so queries can at least match that
				ItemTags.AddTag(ItemData->ItemIDTag);

				// Empty query = everything allowed
				return Def.AllowedItemQuery.IsEmpty() || Def.AllowedItemQuery.Matches(ItemTags);
			}
		}
		return false;
	};

	return Matches(WeaponSlots) || Matches(ArmorSlots);
}


UItemDataAsset* UEquipmentComponent::LoadItemDataByID(const FGameplayTag& ItemID) const
{
	if (!ItemID.IsValid()) return nullptr;
	return UInventoryHelpers::FindItemDataByTag(GetWorld(), ItemID);
}

FText UEquipmentComponent::GetSlotDisplayName(const FGameplayTag& Slot) const
{
	auto PrettyFromTag = [](const FGameplayTag& T)->FText
	{
		const FString S = T.ToString().Replace(TEXT("."), TEXT(" / "));
		return FText::FromString(S);
	};

	for (const FEquipmentSlotDef& D : WeaponSlots)
	{
		if (D.SlotTag == Slot)
			return D.SlotName.IsNone() ? PrettyFromTag(Slot) : FText::FromName(D.SlotName);
	}
	for (const FEquipmentSlotDef& D : ArmorSlots)
	{
		if (D.SlotTag == Slot)
			return D.SlotName.IsNone() ? PrettyFromTag(Slot) : FText::FromName(D.SlotName);
	}
	return PrettyFromTag(Slot);
}

void UEquipmentComponent::GetAllSlotTags(TArray<FGameplayTag>& Out) const
{
	Out.Reset();
	for (const FEquipmentSlotDef& D : WeaponSlots) Out.Add(D.SlotTag);
	for (const FEquipmentSlotDef& D : ArmorSlots)  Out.Add(D.SlotTag);
	Out.Sort([](const FGameplayTag& A, const FGameplayTag& B){ return A.ToString() < B.ToString(); });
	Out.SetNum(Out.Num(), EAllowShrinking::No);
}

bool UEquipmentComponent::IsSlotOccupied(const FGameplayTag& Slot) const
{
	const int32 Idx = FindEquippedIndex(Slot);
	return (Idx != INDEX_NONE) && Equipped[Idx].ItemIDTag.IsValid();
}

UItemDataAsset* UEquipmentComponent::GetEquippedItemDataForSlot(const FGameplayTag& Slot) const
{
	const int32 Idx = FindEquippedIndex(Slot);
	return (Idx != INDEX_NONE) ? LoadItemDataByID(Equipped[Idx].ItemIDTag) : nullptr;
}

void UEquipmentComponent::SetSlot_Internal(const FGameplayTag& Slot, const FGameplayTag& ItemID)
{
	const int32 Idx = FindEquippedIndex(Slot);
	if (Idx == INDEX_NONE)
	{
		Equipped.Add(FEquippedEntry(Slot, ItemID));
	}
	else
	{
		Equipped[Idx].ItemIDTag = ItemID;
	}

	UItemDataAsset* Data = LoadItemDataByID(ItemID);
	OnEquipmentChanged.Broadcast(Slot, Data);
	OnEquippedItemChanged.Broadcast(Slot, Data);
}

void UEquipmentComponent::ClearSlot_Internal(const FGameplayTag& Slot)
{
	const int32 Idx = FindEquippedIndex(Slot);
	if (Idx != INDEX_NONE)
	{
		Equipped[Idx].ItemIDTag = FGameplayTag();
		OnEquipmentChanged.Broadcast(Slot, nullptr);
		OnEquipmentSlotCleared.Broadcast(Slot);
		OnEquippedItemChanged.Broadcast(Slot, nullptr);
		OnEquippedSlotCleared.Broadcast(Slot);
	}
}

UInventoryComponent* UEquipmentComponent::FindPlayerStateInventory() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	const APawn* Pawn = Cast<APawn>(OwnerActor);
	const APlayerState* PS = Pawn ? Pawn->GetPlayerState() : Cast<APlayerState>(OwnerActor);
	if (!PS) return nullptr;

	return PS->FindComponentByClass<UInventoryComponent>();
}

// ---------- Public Actions (with client->server forwarding) ----------
bool UEquipmentComponent::TryEquipByInventoryIndex(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield)
{
	if (!SourceInventory) return false;

	// Pull item by value
	FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	UItemDataAsset* SourceData = Item.ItemData.LoadSynchronous(); // OK to use .Get() if you don't async-load
	if (!SourceData) return false;

	// Check slot acceptance
	if (!IsItemAllowedInSlot(SourceData, SlotTag)) return false;

	// Consume 1 from that inventory slot (your API removes by index)
	if (!SourceInventory->TryRemoveItem(SourceIndex, /*Quantity*/1))
	{
		return false;
	}

	// Record the equipped state
	SetSlot_Internal(SlotTag, SourceData->ItemIDTag);

	// Optional: auto-wield the freshly equipped item
	if (bAlsoWield || bAutoWieldAfterEquip)
	{
		if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(GetOwner()))
		{
			W->TryWieldEquippedInSlot(SlotTag);
		}
	}
	return true;
}

bool UEquipmentComponent::TryEquipByItemIDTag(const FGameplayTag& SlotTag, const FGameplayTag& ItemIDTag, bool bAlsoWield)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		Server_TryEquipByItemIDTag(SlotTag, ItemIDTag, bAlsoWield);
		return true;
	}

	UItemDataAsset* Data = LoadItemDataByID(ItemIDTag);
	if (!Data) return false;
	if (!IsItemAllowedInSlot(Data, SlotTag)) return false;

	SetSlot_Internal(SlotTag, ItemIDTag);

	if (bAlsoWield || bAutoWieldAfterEquip)
	{
		if (UWieldComponent* W = UEquipmentHelperLibrary::GetWieldPS(GetOwner()))
		{
			W->TryWieldEquippedInSlot(SlotTag);
		}
	}

	return true;
}

bool UEquipmentComponent::TryUnequipSlotToInventory(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		Server_TryUnequipSlotToInventory(SlotTag, DestInventory);
		return true;
	}

	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE || !Equipped[Idx].ItemIDTag.IsValid()) return false;

	if (!DestInventory)
	{
		DestInventory = FindPlayerStateInventory();
		if (!DestInventory) return false;
	}

	UItemDataAsset* Data = LoadItemDataByID(Equipped[Idx].ItemIDTag);
	if (!Data) return false;

	// Add back one to the destination inventory
	if (!DestInventory->TryAddItem(Data, /*Quantity*/1))
	{
		return false;
	}

	// Clear slot
	ClearSlot_Internal(SlotTag);
	return true;
}

bool UEquipmentComponent::TryUnequipSlot(const FGameplayTag& SlotTag)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		Server_TryUnequipSlot(SlotTag);
		return true;
	}

	ClearSlot_Internal(SlotTag);
	return true;
}

// ---------- Server RPCs ----------
void UEquipmentComponent::Server_TryEquipByInventoryIndex_Implementation(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield)
{
	TryEquipByInventoryIndex(SlotTag, SourceInventory, SourceIndex, bAlsoWield);
}

void UEquipmentComponent::Server_TryEquipByItemIDTag_Implementation(const FGameplayTag& SlotTag, FGameplayTag ItemIDTag, bool bAlsoWield)
{
	TryEquipByItemIDTag(SlotTag, ItemIDTag, bAlsoWield);
}

void UEquipmentComponent::Server_TryUnequipSlotToInventory_Implementation(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory)
{
	TryUnequipSlotToInventory(SlotTag, DestInventory);
}

void UEquipmentComponent::Server_TryUnequipSlot_Implementation(const FGameplayTag& SlotTag)
{
	TryUnequipSlot(SlotTag);
}

UItemDataAsset* UEquipmentComponent::GetEquippedItemData_Compat(const FGameplayTag& SlotTag) const
{
	return UEquipmentHelperLibrary::GetEquippedItemData(GetOwner(), SlotTag);
}