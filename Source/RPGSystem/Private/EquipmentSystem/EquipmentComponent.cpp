// All rights Reserved Midnight Entertainment Studios LLC
// Source/RPGSystem/Private/EquipmentSystem/EquipmentComponent.cpp

#include "EquipmentSystem/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "Net/UnrealNetwork.h"

UEquipmentComponent::UEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::GetAllSlotDefs(TArray<FEquipmentSlotDef>& OutDefs) const
{
	OutDefs.Reset(WeaponSlotDefs.Num() + ArmorSlotDefs.Num());
	OutDefs.Append(WeaponSlotDefs);
	OutDefs.Append(ArmorSlotDefs);
}

bool UEquipmentComponent::GetSlotDef(const FGameplayTag& SlotTag, FEquipmentSlotDef& OutDef) const
{
	if (!SlotTag.IsValid()) return false;
	for (const FEquipmentSlotDef& D : WeaponSlotDefs)
	{
		if (D.SlotTag.MatchesTagExact(SlotTag)) { OutDef = D; return true; }
	}
	for (const FEquipmentSlotDef& D : ArmorSlotDefs)
	{
		if (D.SlotTag.MatchesTagExact(SlotTag)) { OutDef = D; return true; }
	}
	return false;
}

const FEquipmentSlotDef* UEquipmentComponent::FindSlotDef(const FGameplayTag& SlotTag) const
{
	if (!SlotTag.IsValid()) return nullptr;
	for (const FEquipmentSlotDef& D : WeaponSlotDefs)
	{
		if (D.SlotTag.MatchesTagExact(SlotTag)) return &D;
	}
	for (const FEquipmentSlotDef& D : ArmorSlotDefs)
	{
		if (D.SlotTag.MatchesTagExact(SlotTag)) return &D;
	}
	return nullptr;
}

void UEquipmentComponent::InitializeSlotsFromDefinitions(bool bOnlyIfEmpty)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	// Only fill missing slots unless overridden
	auto TryInit = [&](const FEquipmentSlotDef& Def)
	{
		if (!Def.SlotTag.IsValid()) return;
		const bool bHas = (FindEntry(Def.SlotTag) != nullptr);
		if (bOnlyIfEmpty && bHas) return;

		UItemDataAsset* Data = nullptr;
		if (Def.ItemIDTag.IsValid())
		{
			Data = UInventoryHelpers::FindItemDataByTag(this, Def.ItemIDTag);
		}
		if (!Data && !Def.ItemData.IsNull())
		{
			Data = Def.ItemData.LoadSynchronous();
		}
		if (!Data) return;

		// Write into replicated state without touching an inventory
		FEquippedEntry* Entry = FindOrAddEntry(Def.SlotTag);
		Entry->ItemIDTag = Data->ItemIDTag;
		Entry->ItemData  = Data;
	};

	for (const FEquipmentSlotDef& D : WeaponSlotDefs) { TryInit(D); }
	for (const FEquipmentSlotDef& D : ArmorSlotDefs)  { TryInit(D); }

	// Fire events for current snapshot
	for (const FEquippedEntry& E : Equipped)
	{
		if (UItemDataAsset* D = ResolveData(E))
		{
			OnEquipmentChanged.Broadcast(E.SlotTag, D);
			OnEquippedItemChanged.Broadcast(E.SlotTag, D);
		}
	}
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentComponent, Equipped);
}

static bool TagsEqualExact(const FGameplayTag& A, const FGameplayTag& B)
{
	return A.MatchesTagExact(B);
}

FEquippedEntry* UEquipmentComponent::FindOrAddEntry(const FGameplayTag& SlotTag)
{
	for (FEquippedEntry& E : Equipped)
	{
		if (TagsEqualExact(E.SlotTag, SlotTag))
			return &E;
	}
	const int32 NewIdx = Equipped.AddDefaulted();
	Equipped[NewIdx].SlotTag = SlotTag;
	return &Equipped[NewIdx];
}

const FEquippedEntry* UEquipmentComponent::FindEntry(const FGameplayTag& SlotTag) const
{
	for (const FEquippedEntry& E : Equipped)
	{
		if (TagsEqualExact(E.SlotTag, SlotTag))
			return &E;
	}
	return nullptr;
}

bool UEquipmentComponent::RemoveEntry(const FGameplayTag& SlotTag)
{
	for (int32 i = 0; i < Equipped.Num(); ++i)
	{
		if (TagsEqualExact(Equipped[i].SlotTag, SlotTag))
		{
			Equipped.RemoveAt(i);
			return true;
		}
	}
	return false;
}

UItemDataAsset* UEquipmentComponent::ResolveData(const FEquippedEntry& E) const
{
	if (E.ItemData.IsValid())
		return E.ItemData.Get();
	if (!E.ItemData.IsNull())
		return E.ItemData.LoadSynchronous();
	return nullptr;
}

void UEquipmentComponent::OnRep_Equipped()
{
	for (const FEquippedEntry& E : Equipped)
	{
		if (UItemDataAsset* Data = ResolveData(E))
		{
			OnEquipmentChanged.Broadcast(E.SlotTag, Data);
			OnEquippedItemChanged.Broadcast(E.SlotTag, Data);
		}
		else
		{
			OnEquipmentSlotCleared.Broadcast(E.SlotTag);
			OnEquippedSlotCleared.Broadcast(E.SlotTag);
		}
	}
}

UItemDataAsset* UEquipmentComponent::GetEquippedItemData(const FGameplayTag& SlotTag) const
{
	if (const FEquippedEntry* E = FindEntry(SlotTag))
		return ResolveData(*E);
	return nullptr;
}

bool UEquipmentComponent::IsSlotOccupied(const FGameplayTag& SlotTag) const
{
	return FindEntry(SlotTag) != nullptr;
}

// ---- validation against slot filter (optional) ----
bool UEquipmentComponent::ValidateItemForSlot(const FGameplayTag& SlotTag, const UItemDataAsset* Data) const
{
	if (!Data) return false;

	const FEquipmentSlotDef* Def = FindSlotDef(SlotTag);
	if (!Def || !Def->AcceptRootTag.IsValid())
	{
		// No constraints → allowed
		return true;
	}

	// Accept if item’s ItemIDTag or AdditionalTags match the root filter
	if (Data->ItemIDTag.IsValid() && Data->ItemIDTag.MatchesTag(Def->AcceptRootTag))
		return true;

	// If your UItemDataAsset exposes an AdditionalTags container, check it:
#if 1
	for (const FGameplayTag& T : Data->AdditionalTags)
	{
		if (T.MatchesTag(Def->AcceptRootTag))
			return true;
	}
#endif
	return false;
}

// ---- client wrappers → server ----
bool UEquipmentComponent::TryEquipByInventoryIndex(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex)
{
	AActor* Owner = GetOwner();
	const bool bAuth = Owner && Owner->HasAuthority();
	if (!bAuth)
	{
		Server_TryEquipByInventoryIndex(SlotTag, SourceInventory, SourceIndex);
		return false;
	}
	return Equip_Internal(SlotTag, SourceInventory, SourceIndex);
}

bool UEquipmentComponent::TryUnequipSlotToInventory(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory)
{
	AActor* Owner = GetOwner();
	const bool bAuth = Owner && Owner->HasAuthority();
	if (!bAuth)
	{
		Server_TryUnequipSlotToInventory(SlotTag, DestInventory);
		return false;
	}
	return Unequip_Internal(SlotTag, DestInventory);
}

// ---------- RPC impls ----------
void UEquipmentComponent::Server_TryEquipByInventoryIndex_Implementation(FGameplayTag SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex)
{
	Equip_Internal(SlotTag, SourceInventory, SourceIndex);
}
void UEquipmentComponent::Server_TryUnequipSlotToInventory_Implementation(FGameplayTag SlotTag, UInventoryComponent* DestInventory)
{
	Unequip_Internal(SlotTag, DestInventory);
}

// ---------- Server logic ----------
bool UEquipmentComponent::Equip_Internal(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex)
{
	if (!SourceInventory || !SlotTag.IsValid())
		return false;

	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	if (Item.ItemData.IsNull())
		return false;

	UItemDataAsset* Data = Item.ItemData.LoadSynchronous();
	if (!Data) return false;

	// Enforce optional slot filter
	if (!ValidateItemForSlot(SlotTag, Data))
		return false;

	FEquippedEntry* Entry = FindOrAddEntry(SlotTag);
	Entry->ItemData = Item.ItemData;
	Entry->ItemIDTag = Data->ItemIDTag;

	SourceInventory->TryRemoveItem(SourceIndex, 1);

	OnEquipmentChanged.Broadcast(SlotTag, Data);
	OnEquippedItemChanged.Broadcast(SlotTag, Data);
	return true;
}

bool UEquipmentComponent::Unequip_Internal(const FGameplayTag& SlotTag, UInventoryComponent* /*DestInventory*/)
{
	const FEquippedEntry* E = FindEntry(SlotTag);
	if (!E) return false;

	RemoveEntry(SlotTag);

	OnEquipmentSlotCleared.Broadcast(SlotTag);
	OnEquippedSlotCleared.Broadcast(SlotTag);
	return true;
}
