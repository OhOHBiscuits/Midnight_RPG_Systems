// Source/RPGSystem/Private/EquipmentSystem/EquipmentHelperLibrary.cpp
#include "EquipmentSystem/EquipmentHelperLibrary.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "EquipmentSystem/WieldComponent.h"

static bool SlotHasItem(UEquipmentComponent* E, const FGameplayTag& Slot)
{
	return (E && E->GetEquippedItemData(Slot) != nullptr);
}

// Resolve PS no matter what you pass (Pawn/PC/PS/Owner chain)
static APlayerState* ResolvePS(const AActor* Actor)
{
	if (!Actor) return nullptr;
	if (const APlayerState* PS = Cast<APlayerState>(Actor)) return const_cast<APlayerState*>(PS);
	if (const APawn* P = Cast<APawn>(Actor)) return P->GetPlayerState();
	if (const AController* C = Cast<AController>(Actor)) return C->PlayerState;
	if (const AActor* Owner = Actor->GetOwner()) return ResolvePS(Owner);
	return nullptr;
}

APlayerState* UEquipmentHelperLibrary::GetPlayerStateFromActor(const AActor* Actor) { return ResolvePS(Actor); }

template<typename T>
static T* GetPSComp(const AActor* FromActor)
{
	if (APlayerState* PS = ResolvePS(FromActor)) return PS->FindComponentByClass<T>();
	return nullptr;
}

UInventoryComponent*       UEquipmentHelperLibrary::GetInventoryPS (const AActor* FromActor){ return GetPSComp<UInventoryComponent>(FromActor); }
UEquipmentComponent*       UEquipmentHelperLibrary::GetEquipmentPS (const AActor* FromActor){ return GetPSComp<UEquipmentComponent>(FromActor); }
UDynamicToolbarComponent*  UEquipmentHelperLibrary::GetToolbarPS   (const AActor* FromActor){ return GetPSComp<UDynamicToolbarComponent>(FromActor); }
UWieldComponent*           UEquipmentHelperLibrary::GetWieldPS     (const AActor* FromActor){ return GetPSComp<UWieldComponent>(FromActor); }

// ---------- Assets ----------
UItemDataAsset* UEquipmentHelperLibrary::LoadItemDataByTag(FGameplayTag ItemIDTag, bool bSynchronousLoad)
{
	if (!ItemIDTag.IsValid()) return nullptr;
	return UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, bSynchronousLoad);
}

// ---------- Equipment (by tag) ----------
bool UEquipmentHelperLibrary::EquipItemByTag(AActor* ContextActor, FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor)) return E->TryEquipByItemIDTag(SlotTag, ItemIDTag);
	return false;
}

bool UEquipmentHelperLibrary::UnequipSlot(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor)) return E->TryUnequipSlot(SlotTag);
	return false;
}

UItemDataAsset* UEquipmentHelperLibrary::GetEquippedItemData(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor)) return E->GetEquippedItemData(SlotTag);
	return nullptr;
}

FGameplayTag UEquipmentHelperLibrary::GetEquippedItemIDTag(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UItemDataAsset* Data = GetEquippedItemData(ContextActor, SlotTag)) return Data->ItemIDTag;
	return FGameplayTag();
}

// ---------- Inventory helpers ----------
bool UEquipmentHelperLibrary::GetInventoryItemIDTag(UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag& OutItemIDTag, bool bLoadIfNeeded)
{
	OutItemIDTag = FGameplayTag();
	if (!SourceInventory || SourceIndex < 0) return false;

	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	if (Item.ItemData.IsNull()) return false;

	if (UItemDataAsset* Data = Item.ItemData.Get())
	{
		OutItemIDTag = Data->ItemIDTag;
		return OutItemIDTag.IsValid();
	}

	if (!bLoadIfNeeded) return false;

	TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData;
	if (UItemDataAsset* Loaded = Soft.LoadSynchronous())
	{
		OutItemIDTag = Loaded->ItemIDTag;
		return OutItemIDTag.IsValid();
	}
	return false;
}

bool UEquipmentHelperLibrary::EquipFromInventoryIndexByItemIDTag(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag SlotTag, bool bAlsoWield, bool bLoadIfNeeded)
{
	FGameplayTag ItemID;
	if (!GetInventoryItemIDTag(SourceInventory, SourceIndex, ItemID, bLoadIfNeeded)) return false;
	const bool bReq = EquipItemByTag(ContextActor, SlotTag, ItemID); // server path inside component
	if (bReq && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(SlotTag);
	return bReq;
}

// Index-based path (kept for completeness)
bool UEquipmentHelperLibrary::EquipFromInventorySlot(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag SlotTag, bool bAlsoWield)
{
	if (!ContextActor || !SourceInventory || SourceIndex < 0 || !SlotTag.IsValid()) return false;
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		const bool bReq = E->TryEquipByInventoryIndex(SlotTag, SourceInventory, SourceIndex);
		if (bReq && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(SlotTag);
		return bReq;
	}
	return false;
}

// ---------- Ability bridge (TAG-ONLY) ----------
void UEquipmentHelperLibrary::SendEquipByItemIDEvent(AActor* AvatarActor, FGameplayTag EquipEventTag, FGameplayTag ItemIDTag, FGameplayTag SlotTag)
{
	if (!AvatarActor || !EquipEventTag.IsValid() || !ItemIDTag.IsValid() || !SlotTag.IsValid()) return;

	FGameplayEventData Payload;
	Payload.EventTag   = EquipEventTag;
	Payload.Instigator = AvatarActor;
	Payload.Target     = AvatarActor;

	// Tags-only: ItemID in InstigatorTags, Slot in TargetTags
	Payload.InstigatorTags.AddTag(ItemIDTag);
	Payload.TargetTags.AddTag(SlotTag);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, EquipEventTag, Payload);
}

bool UEquipmentHelperLibrary::ParseEquipByItemIDEvent(const FGameplayEventData& EventData, FGameplayTag& OutItemIDTag, FGameplayTag& OutSlotTag)
{
	OutItemIDTag = FGameplayTag();
	OutSlotTag   = FGameplayTag();

	for (auto It = EventData.InstigatorTags.CreateConstIterator(); It; ++It) { OutItemIDTag = *It; break; }
	for (auto It = EventData.TargetTags.CreateConstIterator();     It; ++It) { OutSlotTag   = *It; break; }

	return OutItemIDTag.IsValid() && OutSlotTag.IsValid();
}

// ---------- Wield / Holster ----------
bool UEquipmentHelperLibrary::WieldEquippedSlot(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->TryWieldEquippedInSlot(SlotTag);
	return false;
}
bool UEquipmentHelperLibrary::ToggleHolster (AActor* ContextActor) { if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->TryToggleHolster(); return false; }
bool UEquipmentHelperLibrary::Holster       (AActor* ContextActor) { if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->TryHolster();      return false; }
bool UEquipmentHelperLibrary::Unholster     (AActor* ContextActor) { if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->TryUnholster();    return false; }
bool UEquipmentHelperLibrary::SetWieldProfile(AActor* ContextActor, FGameplayTag ProfileTag)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->TrySetActiveWieldProfile(ProfileTag);
	return false;
}
bool UEquipmentHelperLibrary::IsHolstered   (AActor* ContextActor) { if (UWieldComponent* W = GetWieldPS(ContextActor)) return W->IsHolstered(); return true; }

// ---------- Toolbar / Zones ----------
void UEquipmentHelperLibrary::ToolbarCycleNext(AActor* ContextActor)  { if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) T->TryCycleNext(); }
void UEquipmentHelperLibrary::ToolbarCyclePrev(AActor* ContextActor)  { if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) T->TryCyclePrev(); }
void UEquipmentHelperLibrary::NotifyZoneTags(AActor* ContextActor, const FGameplayTagContainer& ZoneTags) { if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) T->TryNotifyZoneTagsUpdated(ZoneTags); }
void UEquipmentHelperLibrary::SetCombatState(AActor* ContextActor, bool bInCombat)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) T->TrySetCombatState(bInCombat);
	if (UWieldComponent* W = GetWieldPS(ContextActor))            W->TrySetCombatState(bInCombat);
}
TArray<FGameplayTag> UEquipmentHelperLibrary::GetToolbarItemIDs(AActor* ContextActor) { if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) return T->ToolbarItemIDs; return {}; }
int32 UEquipmentHelperLibrary::GetToolbarActiveIndex(AActor* ContextActor)            { if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor)) return T->ActiveIndex;   return INDEX_NONE; }

// ---------- “best slot” helpers ----------
bool UEquipmentHelperLibrary::DetermineBestEquipSlotForItemID(AActor* /*ContextActor*/, FGameplayTag ItemIDTag, FGameplayTag& OutSlotTag)
{
	OutSlotTag = FGameplayTag();
	if (!ItemIDTag.IsValid()) return false;

	UItemDataAsset* Data = LoadItemDataByTag(ItemIDTag, /*sync*/true);
	if (!Data) return false;

	if (Data->PreferredEquipSlots.Num() > 0)
	{
		OutSlotTag = Data->PreferredEquipSlots[0];
		return OutSlotTag.IsValid();
	}
	return false;
}

static FGameplayTag ChooseFirstEmptyOrSecond(UEquipmentComponent* E, const TArray<FGameplayTag>& Prefs)
{
	if (!E || Prefs.Num() == 0) return FGameplayTag();
	if (!SlotHasItem(E, Prefs[0])) return Prefs[0];
	if (Prefs.Num() > 1 && !SlotHasItem(E, Prefs[1])) return Prefs[1];
	return Prefs[0]; // both full -> primary for swap
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield)
{
	if (!ContextActor || !SourceInventory || SourceIndex < 0) return false;

	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	if (Item.ItemData.IsNull()) return false;

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) { TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData; Data = Soft.LoadSynchronous(); }
	if (!Data) return false;

	UEquipmentComponent* Equip = GetEquipmentPS(ContextActor);
	if (!Equip) return false;

	TArray<FGameplayTag> Prefs = Data->PreferredEquipSlots;
	if (Prefs.Num() == 0) return false;

	const FGameplayTag Target = ChooseFirstEmptyOrSecond(Equip, Prefs);

	if (!SlotHasItem(Equip, Target))
	{
		const bool bReq = Equip->TryEquipByInventoryIndex(Target, SourceInventory, SourceIndex);
		if (bReq && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(Target);
		return bReq;
	}

	const bool bUnequipped = Equip->TryUnequipSlotToInventory(Target, SourceInventory);
	if (!bUnequipped) return false;

	const bool bEquipped = Equip->TryEquipByInventoryIndex(Target, SourceInventory, SourceIndex);
	if (bEquipped && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(Target);
	return bEquipped;
}

void UEquipmentHelperLibrary::FilterPreferredSlotsByRoot(UItemDataAsset* ItemData, FGameplayTag RootTag, TArray<FGameplayTag>& OutSlots)
{
	OutSlots.Reset();
	if (!ItemData) return;

	if (!RootTag.IsValid())
	{
		OutSlots = ItemData->PreferredEquipSlots;
		return;
	}

	for (const FGameplayTag& T : ItemData->PreferredEquipSlots)
	{
		if (T.IsValid() && T.MatchesTag(RootTag)) OutSlots.Add(T);
	}
}

static FGameplayTag ChooseEmptyOrPrimary(UEquipmentComponent* E, const TArray<FGameplayTag>& Prefs)
{
	if (!E || Prefs.Num() == 0) return FGameplayTag();
	for (const FGameplayTag& T : Prefs) if (T.IsValid() && (E->GetEquippedItemData(T) == nullptr)) return T;
	return Prefs[0];
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndexUnder(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RootTag, bool bAlsoWield)
{
	if (!ContextActor || !SourceInventory || SourceIndex < 0) return false;

	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	if (Item.ItemData.IsNull()) return false;

	UItemDataAsset* Data = Item.ItemData.Get();
	if (!Data) { TSoftObjectPtr<UItemDataAsset> Soft = Item.ItemData; Data = Soft.LoadSynchronous(); }
	if (!Data) return false;

	UEquipmentComponent* Equip = GetEquipmentPS(ContextActor);
	if (!Equip) return false;

	TArray<FGameplayTag> Prefs;
	FilterPreferredSlotsByRoot(Data, RootTag, Prefs);
	if (Prefs.Num() == 0) return false;

	const FGameplayTag Target = ChooseEmptyOrPrimary(Equip, Prefs);
	if (!Target.IsValid()) return false;

	if (Equip->GetEquippedItemData(Target) == nullptr)
	{
		const bool bReq = Equip->TryEquipByInventoryIndex(Target, SourceInventory, SourceIndex);
		if (bReq && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(Target);
		return bReq;
	}

	const bool bUnequipped = Equip->TryUnequipSlotToInventory(Target, SourceInventory);
	if (!bUnequipped) return false;

	const bool bEquipped = Equip->TryEquipByInventoryIndex(Target, SourceInventory, SourceIndex);
	if (bEquipped && bAlsoWield) if (UWieldComponent* W = GetWieldPS(ContextActor)) W->TryWieldEquippedInSlot(Target);
	return bEquipped;
}

APlayerState* UEquipmentHelperLibrary::ResolvePlayerState(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;
	if (APlayerState* PS = Cast<APlayerState>(ContextActor)) return PS;
	if (APawn*      P  = Cast<APawn>(ContextActor))          return P->GetPlayerState();
	if (APlayerController* PC = Cast<APlayerController>(ContextActor)) return PC->PlayerState;
	return ContextActor->GetTypedOuter<APlayerState>();
}
