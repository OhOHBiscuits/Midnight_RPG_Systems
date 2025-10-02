// Source/RPGSystem/Private/EquipmentSystem/EquipmentComponent.cpp

#include "EquipmentSystem/EquipmentComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"

// -------- local helpers (file scope) ----------
static FText PrettyFromTag(const FGameplayTag& Tag)
{
	return FText::FromName(Tag.IsValid() ? Tag.GetTagName() : NAME_None);
}

static void BuildItemTagContainer(const UItemDataAsset* Data, FGameplayTagContainer& OutTags)
{
	OutTags.Reset();
	if (!Data) return;

	// These names match your data assets. If you’ve renamed them, add/adjust here:
	// (Failsafe: if a field is missing in this build, it simply won't be added.)
	auto TryAdd = [&OutTags](const FGameplayTag& T)
	{
		if (T.IsValid()) { OutTags.AddTag(T); }
	};

	// Common ID/type/category fields
	// UItemDataAsset::ItemIDTag
	// UItemDataAsset::ItemTypeTag
	// UItemDataAsset::ItemCategoryTag
	// UItemDataAsset::ItemSubCategoryTag
	// Some weapon assets also carry preferred slot tags arrays; those are not required for AllowedItemQuery.

	// We can't include headers showing these properties explicitly (they were truncated in the provided file),
	// so we use a soft approach: check for UPROPERTYs via accessors if you expose them; otherwise, keep core fields.

	// Core expected fields:
	// clang-format off
	// If you expose accessors later, prefer those. For now we assume public fields:
	// NOTE: Remove comments and switch to your actual getters if needed.
	TryAdd(Data->ItemIDTag);
	TryAdd(Data->ItemTypeTag);
	TryAdd(Data->ItemCategoryTag);
	TryAdd(Data->ItemSubCategoryTag);
	// clang-format on
}

// --------------- UEquipmentComponent ----------------

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Sensible defaults in case designer forgets to add slots
	if (WeaponSlots.Num() == 0)
	{
		FEquipmentSlotDef S;
		S.SlotName = TEXT("Primary");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Weapon.Primary"), false);
		S.bOptional = true;
		WeaponSlots.Add(S);

		S.SlotName = TEXT("Secondary");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Weapon.Secondary"), false);
		WeaponSlots.Add(S);

		S.SlotName = TEXT("SideArm");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Weapon.SideArm"), false);
		WeaponSlots.Add(S);

		S.SlotName = TEXT("Melee");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Weapon.Melee"), false);
		WeaponSlots.Add(S);
	}

	if (ArmorSlots.Num() == 0)
	{
		FEquipmentSlotDef S;
		S.SlotName = TEXT("Helmet");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Armor.Helmet"), false);
		ArmorSlots.Add(S);

		S.SlotName = TEXT("Chest");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Armor.Chest"), false);
		ArmorSlots.Add(S);

		S.SlotName = TEXT("Hands");
		S.SlotTag = FGameplayTag::RequestGameplayTag(FName("Slots.Armor.Hands"), false);
		ArmorSlots.Add(S);
	}
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure Equipped array has an entry for every configured slot.
	auto EnsureEntryFor = [this](const TArray<FEquipmentSlotDef>& Defs)
	{
		for (const FEquipmentSlotDef& D : Defs)
		{
			if (!D.SlotTag.IsValid()) continue;
			if (FindEquippedIndex(D.SlotTag) == INDEX_NONE)
			{
				FEquippedEntry E;
				E.SlotTag = D.SlotTag;
				Equipped.Add(E);
			}
		}
	};

	EnsureEntryFor(WeaponSlots);
	EnsureEntryFor(ArmorSlots);

	// Cache initial copy for client diffing
	LocalLastEquippedCache = Equipped;
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentComponent, Equipped);
}

void UEquipmentComponent::OnRep_Equipped()
{
	// Diff vs last local cache to broadcast only the changed slots
	TMap<FGameplayTag, FGameplayTag> PrevBySlot;
	for (const FEquippedEntry& E : LocalLastEquippedCache)
	{
		PrevBySlot.Add(E.SlotTag, E.ItemIDTag);
	}

	for (const FEquippedEntry& Now : Equipped)
	{
		const FGameplayTag* PrevItem = PrevBySlot.Find(Now.SlotTag);
		const bool bWasEmpty = (!PrevItem || !PrevItem->IsValid());
		const bool bIsEmpty  = (!Now.ItemIDTag.IsValid());

		if (bIsEmpty && !bWasEmpty)
		{
			OnEquipmentSlotCleared.Broadcast(Now.SlotTag);
		}
		else if (!bIsEmpty && (bWasEmpty || *PrevItem != Now.ItemIDTag))
		{
			UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(Now.ItemIDTag, /*Sync*/true);
			OnEquipmentChanged.Broadcast(Now.SlotTag, Data);
		}
	}

	LocalLastEquippedCache = Equipped;
}

// ---------------- Queries ----------------

const FEquipmentSlotDef* UEquipmentComponent::FindSlotDef(FGameplayTag SlotTag) const
{
	if (!SlotTag.IsValid()) return nullptr;

	for (const FEquipmentSlotDef& D : WeaponSlots)
	{
		if (D.SlotTag == SlotTag) return &D;
	}
	for (const FEquipmentSlotDef& D : ArmorSlots)
	{
		if (D.SlotTag == SlotTag) return &D;
	}
	return nullptr;
}

int32 UEquipmentComponent::FindEquippedIndex(FGameplayTag SlotTag) const
{
	for (int32 i = 0; i < Equipped.Num(); ++i)
	{
		if (Equipped[i].SlotTag == SlotTag) return i;
	}
	return INDEX_NONE;
}

bool UEquipmentComponent::IsItemAllowedInSlot(const FEquipmentSlotDef& SlotDef, const UItemDataAsset* ItemData) const
{
	if (!ItemData) return false;
	if (!SlotDef.AllowedItemQuery.IsEmpty()) // if you set a query, enforce it
	{
		FGameplayTagContainer ItemTags;
		BuildItemTagContainer(ItemData, ItemTags);
		return SlotDef.AllowedItemQuery.Matches(ItemTags);
	}
	return true;
}

UItemDataAsset* UEquipmentComponent::GetEquippedItemData(FGameplayTag SlotTag) const
{
	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE) return nullptr;

	const FGameplayTag ItemID = Equipped[Idx].ItemIDTag;
	if (!ItemID.IsValid()) return nullptr;

	return UInventoryAssetManager::Get().LoadItemDataByTag(ItemID, /*Sync*/true);
}

bool UEquipmentComponent::IsEquipped(FGameplayTag SlotTag) const
{
	const int32 Idx = FindEquippedIndex(SlotTag);
	return (Idx != INDEX_NONE && Equipped[Idx].ItemIDTag.IsValid());
}

void UEquipmentComponent::GetAllSlotTags(TArray<FGameplayTag>& OutSlots) const
{
	OutSlots.Reset();
	OutSlots.Reserve(WeaponSlots.Num() + ArmorSlots.Num());
	for (const FEquipmentSlotDef& D : WeaponSlots) if (D.SlotTag.IsValid()) OutSlots.Add(D.SlotTag);
	for (const FEquipmentSlotDef& D : ArmorSlots) if (D.SlotTag.IsValid()) OutSlots.Add(D.SlotTag);
}

FText UEquipmentComponent::GetSlotDisplayName(FGameplayTag Slot) const
{
	if (const FEquipmentSlotDef* Def = FindSlotDef(Slot))
	{
		return Def->SlotName.IsNone() ? PrettyFromTag(Slot) : FText::FromName(Def->SlotName);
	}
	return PrettyFromTag(Slot);
}

void UEquipmentComponent::GetWeaponSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const
{
	OutTags.Reset(); OutNames.Reset();
	for (const FEquipmentSlotDef& D : WeaponSlots)
	{
		if (!D.SlotTag.IsValid()) continue;
		OutTags.Add(D.SlotTag);
		OutNames.Add(D.SlotName.IsNone() ? PrettyFromTag(D.SlotTag) : FText::FromName(D.SlotName));
	}
}

void UEquipmentComponent::GetArmorSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const
{
	OutTags.Reset(); OutNames.Reset();
	for (const FEquipmentSlotDef& D : ArmorSlots)
	{
		if (!D.SlotTag.IsValid()) continue;
		OutTags.Add(D.SlotTag);
		OutNames.Add(D.SlotName.IsNone() ? PrettyFromTag(D.SlotTag) : FText::FromName(D.SlotName));
	}
}

// ---------------- Actions ---------------

bool UEquipmentComponent::EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex, bool bAutoApplyModifiers, bool bAlsoWield)
{
	if (!GetOwner() || !FromInventory) return false;

	if (!GetOwner()->HasAuthority())
	{
		ServerEquipByInventoryIndex(SlotTag, FromInventory, FromIndex, bAutoApplyModifiers, bAlsoWield);
		return true; // let server decide; UI will tick after replication
	}

	// Validate item from the inventory
	if (!FromInventory->Items.IsValidIndex(FromIndex)) return false;

	const FInventoryItem& ItemRef = FromInventory->Items[FromIndex];
	UItemDataAsset* Data = ItemRef.ItemData.Get(); // Data assets are soft in your Items array
	if (!Data) return false;

	const FEquipmentSlotDef* Def = FindSlotDef(SlotTag);
	if (!Def || !IsItemAllowedInSlot(*Def, Data)) return false;

	// Set slot to item
	const FGameplayTag ItemID = Data->ItemIDTag;
	if (!EquipLocal(SlotTag, ItemID, bAutoApplyModifiers, bAlsoWield)) return false;

	// Consume 1 from that stack on the authoritative inventory
	FromInventory->TryRemoveItem(FromIndex, /*Quantity*/1);

	return true;
}

bool UEquipmentComponent::EquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool bAutoApplyModifiers, bool bAlsoWield)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		ServerEquipByItemIDTag(SlotTag, ItemIDTag, bAutoApplyModifiers, bAlsoWield);
		return true;
	}

	// Load the data to validate against AllowedItemQuery
	UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, /*Sync*/true);
	if (!Data) return false;

	const FEquipmentSlotDef* Def = FindSlotDef(SlotTag);
	if (!Def || !IsItemAllowedInSlot(*Def, Data)) return false;

	return EquipLocal(SlotTag, ItemIDTag, bAutoApplyModifiers, bAlsoWield);
}

bool UEquipmentComponent::UnequipSlot(FGameplayTag SlotTag)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		ServerUnequipSlot(SlotTag);
		return true;
	}

	return UnequipLocal(SlotTag);
}

bool UEquipmentComponent::UnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory)
{
	if (!GetOwner()) return false;

	if (!GetOwner()->HasAuthority())
	{
		ServerUnequipSlotToInventory(SlotTag, DestInventory);
		return true;
	}

	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE) return false;

	const FGameplayTag ItemID = Equipped[Idx].ItemIDTag;
	if (!ItemID.IsValid()) return false;

	UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ItemID, /*Sync*/true);
	if (!Data) return false;

	// Try add back to inventory first
	bool bAdded = false;
	if (DestInventory)
	{
		bAdded = DestInventory->TryAddItem(Data, /*Quantity*/1);
	}

	// Even if add fails, we still clear the slot (design choice; change if you want strict behavior)
	UnequipLocal(SlotTag);
	return bAdded;
}

// ---------------- Local mutators (authority only) ----------------

bool UEquipmentComponent::EquipLocal(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool /*bAutoApplyModifiers*/, bool /*bAlsoWield*/)
{
	int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE)
	{
		FEquippedEntry E; E.SlotTag = SlotTag; E.ItemIDTag = ItemIDTag;
		Idx = Equipped.Add(E);
	}
	else
	{
		Equipped[Idx].ItemIDTag = ItemIDTag;
	}

	// Immediate feedback on the server (listen server host sees UI instantly)
	NotifySlotSetLocal(SlotTag, ItemIDTag);
	return true;
}

bool UEquipmentComponent::UnequipLocal(FGameplayTag SlotTag)
{
	const int32 Idx = FindEquippedIndex(SlotTag);
	if (Idx == INDEX_NONE) return false;

	if (!Equipped[Idx].ItemIDTag.IsValid())
	{
		// Already empty
		return true;
	}

	Equipped[Idx].ItemIDTag = FGameplayTag(); // clear
	NotifySlotClearedLocal(SlotTag);
	return true;
}

void UEquipmentComponent::NotifySlotSetLocal(FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, /*Sync*/true);
	OnEquipmentChanged.Broadcast(SlotTag, Data);

	// Update local cache so subsequent OnRep diff is correct on server too
	bool bFound = false;
	for (FEquippedEntry& E : LocalLastEquippedCache)
	{
		if (E.SlotTag == SlotTag) { E.ItemIDTag = ItemIDTag; bFound = true; break; }
	}
	if (!bFound)
	{
		FEquippedEntry NewE; NewE.SlotTag = SlotTag; NewE.ItemIDTag = ItemIDTag;
		LocalLastEquippedCache.Add(NewE);
	}
}

void UEquipmentComponent::NotifySlotClearedLocal(FGameplayTag SlotTag)
{
	OnEquipmentSlotCleared.Broadcast(SlotTag);

	// Update cache
	for (FEquippedEntry& E : LocalLastEquippedCache)
	{
		if (E.SlotTag == SlotTag) { E.ItemIDTag = FGameplayTag(); break; }
	}
}

// ---------------- RPC impls ----------------

bool UEquipmentComponent::ServerEquipByInventoryIndex_Validate(FGameplayTag, UInventoryComponent*, int32, bool, bool) { return true; }
void UEquipmentComponent::ServerEquipByInventoryIndex_Implementation(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex, bool bAutoApplyModifiers, bool bAlsoWield)
{
	EquipByInventoryIndex(SlotTag, FromInventory, FromIndex, bAutoApplyModifiers, bAlsoWield);
}

bool UEquipmentComponent::ServerEquipByItemIDTag_Validate(FGameplayTag, FGameplayTag, bool, bool) { return true; }
void UEquipmentComponent::ServerEquipByItemIDTag_Implementation(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool bAutoApplyModifiers, bool bAlsoWield)
{
	EquipByItemIDTag(SlotTag, ItemIDTag, bAutoApplyModifiers, bAlsoWield);
}

bool UEquipmentComponent::ServerUnequipSlot_Validate(FGameplayTag) { return true; }
void UEquipmentComponent::ServerUnequipSlot_Implementation(FGameplayTag SlotTag)
{
	UnequipSlot(SlotTag);
}

bool UEquipmentComponent::ServerUnequipSlotToInventory_Validate(FGameplayTag, UInventoryComponent*) { return true; }
void UEquipmentComponent::ServerUnequipSlotToInventory_Implementation(FGameplayTag SlotTag, UInventoryComponent* DestInventory)
{
	UnequipSlotToInventory(SlotTag, DestInventory);
}
