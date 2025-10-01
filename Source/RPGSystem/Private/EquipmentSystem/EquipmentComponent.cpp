// EquipmentComponent.cpp

#include "EquipmentSystem/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryComponent.h" 
#include "Inventory/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"

static FText PrettyFromTag(const FGameplayTag& Slot)
{
	if (!Slot.IsValid()) return FText::GetEmpty();
	const FString Full = Slot.ToString();
	int32 Dot = INDEX_NONE;
	if (Full.FindLastChar('.', Dot) && Dot + 1 < Full.Len())
	{
		const FString Tail = Full.Mid(Dot + 1).Replace(TEXT("_"), TEXT(" "));
		return FText::FromString(Tail);
	}
	return FText::FromString(Full);
}

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildSlotTagCaches();
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquipmentComponent, Equipped);
	// DOREPLIFETIME_CONDITION(UEquipmentComponent, SlotNameTable, COND_OwnerOnly); // example
}

void UEquipmentComponent::RebuildSlotTagCaches()
{
	WeaponSlotTags.Reset();
	ArmorSlotTags.Reset();

	WeaponSlotTags.Reserve(WeaponSlots.Num());
	for (const FEquipmentSlotDef& Def : WeaponSlots)
	{
		if (Def.SlotTag.IsValid())
		{
			WeaponSlotTags.Add(Def.SlotTag);
		}
	}

	ArmorSlotTags.Reserve(ArmorSlots.Num());
	for (const FEquipmentSlotDef& Def : ArmorSlots)
	{
		if (Def.SlotTag.IsValid())
		{
			ArmorSlotTags.Add(Def.SlotTag);
		}
	}
}

const FEquipmentSlotDef* UEquipmentComponent::FindSlotDef(FGameplayTag SlotTag) const
{
	for (const FEquipmentSlotDef& Def : WeaponSlots)
	{
		if (Def.SlotTag == SlotTag) return &Def;
	}
	for (const FEquipmentSlotDef& Def : ArmorSlots)
	{
		if (Def.SlotTag == SlotTag) return &Def;
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
			if (UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(E.ItemIDTag, /*sync*/true))
			{
				OnEquippedItemChanged.Broadcast(E.SlotTag, Data);
				OnEquipmentChanged.Broadcast(E.SlotTag, Data);
			}
		}
		else
		{
			OnEquippedSlotCleared.Broadcast(E.SlotTag);
			OnEquipmentSlotCleared.Broadcast(E.SlotTag);
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

	return UInventoryAssetManager::Get().LoadItemDataByTag(E.ItemIDTag, /*sync*/true);
}

bool UEquipmentComponent::EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex)
{
	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority()) return TryEquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
	}

	if (!FromInventory || !FromInventory->GetItems().IsValidIndex(FromIndex)) return false;

	const FInventoryItem Item = FromInventory->GetItem(FromIndex);
	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) return false;

	const FEquipmentSlotDef* SlotDef = FindSlotDef(SlotTag);
	if (!SlotDef || !IsItemAllowedInSlot(*SlotDef, Data)) return false;

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
	OnEquipmentChanged.Broadcast(SlotTag, Data);
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

	UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, /*sync*/true);
	if (!Data || !IsItemAllowedInSlot(*SlotDef, Data)) return false;

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
	OnEquipmentChanged.Broadcast(SlotTag, Data);
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
		Prev = UInventoryAssetManager::Get().LoadItemDataByTag(Equipped[Idx].ItemIDTag, /*sync*/true);
	}

	Equipped[Idx].ItemIDTag = FGameplayTag();

	if (bAutoApplyItemModifiers && Prev) ApplyItemModifiers(Prev, false);

	OnEquippedSlotCleared.Broadcast(SlotTag);
	OnEquipmentSlotCleared.Broadcast(SlotTag);
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

bool UEquipmentComponent::TryUnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInv)
{
	if (!DestInv) return TryUnequipSlot(SlotTag);

	AActor* OwnerActor = GetOwner();
	AController* Ctrl = OwnerActor ? OwnerActor->GetInstigatorController() : nullptr;

	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		Server_UnequipSlotToInventory(SlotTag, DestInv, Ctrl);
		return true;
	}

	UItemDataAsset* Data = GetEquippedItemData(SlotTag);
	if (!Data) return false;

	int32 AddedIndex = INDEX_NONE;
	const bool bAdded = UInventoryHelpers::TryAddByItemIDTag(DestInv, Data->ItemIDTag, /*Qty*/1, AddedIndex);
	if (!bAdded) return false;

	return TryUnequipSlot(SlotTag);
}

// RPCs
bool UEquipmentComponent::ServerEquipByInventoryIndex_Validate(FGameplayTag, UInventoryComponent*, int32) { return true; }
void UEquipmentComponent::ServerEquipByInventoryIndex_Implementation(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex)
{
	EquipByInventoryIndex(SlotTag, FromInventory, FromIndex);
}

bool UEquipmentComponent::ServerEquipByItemIDTag_Validate(FGameplayTag, FGameplayTag) { return true; }
void UEquipmentComponent::ServerEquipByItemIDTag_Implementation(FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	EquipByItemIDTag(SlotTag, ItemIDTag);
}

bool UEquipmentComponent::ServerUnequipSlot_Validate(FGameplayTag) { return true; }
void UEquipmentComponent::ServerUnequipSlot_Implementation(FGameplayTag SlotTag)
{
	UnequipSlot(SlotTag);
}

void UEquipmentComponent::Server_UnequipSlotToInventory_Implementation(FGameplayTag SlotTag, UInventoryComponent* DestInventory, AController* /*Requestor*/)
{
	TryUnequipSlotToInventory(SlotTag, DestInventory);
}

void UEquipmentComponent::GetWeaponSlotTags(TArray<FGameplayTag>& OutSlots) const
{
	OutSlots = WeaponSlotTags;
}

void UEquipmentComponent::GetArmorSlotTags(TArray<FGameplayTag>& OutSlots) const
{
	OutSlots = ArmorSlotTags;
}

void UEquipmentComponent::GetAllSlotTags(TArray<FGameplayTag>& OutSlots) const
{
	OutSlots.Reset();

	TSet<FGameplayTag> Seen;
	auto AddUnique = [&OutSlots, &Seen](const TArray<FGameplayTag>& In)
	{
		for (const FGameplayTag& T : In)
		{
			if (T.IsValid() && !Seen.Contains(T))
			{
				Seen.Add(T);
				OutSlots.Add(T);
			}
		}
	};

	AddUnique(WeaponSlotTags);
	AddUnique(ArmorSlotTags);
}

FText UEquipmentComponent::GetSlotDisplayName(FGameplayTag Slot) const
{
	for (const FEquipmentSlotDef& Def : WeaponSlots)
	{
		if (Def.SlotTag == Slot)
		{
			return Def.SlotName.IsNone() ? PrettyFromTag(Slot) : FText::FromName(Def.SlotName);
		}
	}
	for (const FEquipmentSlotDef& Def : ArmorSlots)
	{
		if (Def.SlotTag == Slot)
		{
			return Def.SlotName.IsNone() ? PrettyFromTag(Slot) : FText::FromName(Def.SlotName);
		}
	}
	return PrettyFromTag(Slot);
}

void UEquipmentComponent::GetWeaponSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const
{
	OutTags.Reset(); OutNames.Reset();
	OutTags.Reserve(WeaponSlots.Num());
	OutNames.Reserve(WeaponSlots.Num());

	for (const FEquipmentSlotDef& Def : WeaponSlots)
	{
		if (Def.SlotTag.IsValid())
		{
			OutTags.Add(Def.SlotTag);
			OutNames.Add(Def.SlotName.IsNone() ? PrettyFromTag(Def.SlotTag) : FText::FromName(Def.SlotName));
		}
	}
}

void UEquipmentComponent::GetArmorSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const
{
	OutTags.Reset(); OutNames.Reset();
	OutTags.Reserve(ArmorSlots.Num());
	OutNames.Reserve(ArmorSlots.Num());

	for (const FEquipmentSlotDef& Def : ArmorSlots)
	{
		if (Def.SlotTag.IsValid())
		{
			OutTags.Add(Def.SlotTag);
			OutNames.Add(Def.SlotName.IsNone() ? PrettyFromTag(Def.SlotTag) : FText::FromName(Def.SlotName));
		}
	}
}

bool UEquipmentComponent::GetSlotDefByTag(FGameplayTag Slot, FEquipmentSlotDef& OutDef) const
{
	for (const FEquipmentSlotDef& Def : WeaponSlots)
	{
		if (Def.SlotTag == Slot) { OutDef = Def; return true; }
	}
	for (const FEquipmentSlotDef& Def : ArmorSlots)
	{
		if (Def.SlotTag == Slot) { OutDef = Def; return true; }
	}
	return false;
}

bool UEquipmentComponent::DoesSlotAllowItem(FGameplayTag Slot, const FGameplayTagContainer& ItemTags) const
{
	FEquipmentSlotDef Def;
	if (!GetSlotDefByTag(Slot, Def)) return true;

	if (!Def.AllowedItemQuery.IsEmpty())
	{
		return Def.AllowedItemQuery.Matches(ItemTags);
	}
	return true;
}

void UEquipmentComponent::ApplyItemModifiers_Implementation(UItemDataAsset* /*ItemData*/, bool /*bApply*/)
{
	// hook for GAS/GSC ability sets, attributes, etc.
}



void UEquipmentComponent::NotifySlotSet(const FGameplayTag& SlotTag, UItemDataAsset* ItemData)
{
	// local broadcast + network
	OnEquipmentChanged.Broadcast(SlotTag, ItemData);
	Multicast_SlotSet(SlotTag, ItemData);
}

void UEquipmentComponent::NotifySlotCleared(const FGameplayTag& SlotTag)
{
	OnEquipmentSlotCleared.Broadcast(SlotTag);
	Multicast_SlotCleared(SlotTag);
}

void UEquipmentComponent::LocalNotify_SlotSet(FGameplayTag SlotTag, UItemDataAsset* ItemData)
{
	OnEquipmentChanged.Broadcast(SlotTag, ItemData);
}

void UEquipmentComponent::LocalNotify_SlotCleared(FGameplayTag SlotTag)
{
	OnEquipmentSlotCleared.Broadcast(SlotTag);
}

void UEquipmentComponent::Multicast_SlotSet_Implementation(FGameplayTag SlotTag, UItemDataAsset* ItemData)
{
	// Runs on server + all clients
	OnEquipmentChanged.Broadcast(SlotTag, ItemData);
}

void UEquipmentComponent::Multicast_SlotCleared_Implementation(FGameplayTag SlotTag)
{
	// Runs on server + all clients
	OnEquipmentSlotCleared.Broadcast(SlotTag);
}

APlayerState* UEquipmentHelperLibrary::ResolvePlayerState(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;
	if (APlayerState* AsPS = Cast<APlayerState>(ContextActor)) return AsPS;
	if (APawn* AsPawn = Cast<APawn>(ContextActor)) return AsPawn->GetPlayerState();
	if (APlayerController* AsPC = Cast<APlayerController>(ContextActor)) return AsPC->PlayerState;
	// try the owner chain as a last resort
	return ContextActor->GetTypedOuter<APlayerState>();
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePlayerState(ContextActor))
	{
		return PS->FindComponentByClass<UEquipmentComponent>();
	}
	return nullptr;
}

UInventoryComponent* UEquipmentHelperLibrary::GetInventoryPS(AActor* ContextActor)
{
	if (APlayerState* PS = ResolvePlayerState(ContextActor))
	{
		return PS->FindComponentByClass<UInventoryComponent>();
	}
	return nullptr;
}