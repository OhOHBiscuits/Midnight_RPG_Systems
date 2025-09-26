// Core/EquipmentHelperLibrary.cpp

#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "EquipmentSystem/WieldComponent.h"

static APlayerState* ResolvePS(const AActor* Actor)
{
	if (!Actor) return nullptr;
	if (const APlayerState* PS = Cast<APlayerState>(Actor)) return const_cast<APlayerState*>(PS);
	if (const APawn* P = Cast<APawn>(Actor)) return P->GetPlayerState();
	if (const AController* C = Cast<AController>(Actor)) return C->PlayerState;
	if (const AActor* Owner = Actor->GetOwner()) return ResolvePS(Owner);
	return nullptr;
}

APlayerState* UEquipmentHelperLibrary::GetPlayerStateFromActor(const AActor* Actor)
{
	return ResolvePS(Actor);
}

template<typename T>
static T* GetPSComp(const AActor* FromActor)
{
	if (APlayerState* PS = ResolvePS(FromActor))
	{
		return PS->FindComponentByClass<T>();
	}
	return nullptr;
}

UInventoryComponent* UEquipmentHelperLibrary::GetInventoryPS(const AActor* FromActor)
{
	return GetPSComp<UInventoryComponent>(FromActor);
}
UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(const AActor* FromActor)
{
	return GetPSComp<UEquipmentComponent>(FromActor);
}
UDynamicToolbarComponent* UEquipmentHelperLibrary::GetToolbarPS(const AActor* FromActor)
{
	return GetPSComp<UDynamicToolbarComponent>(FromActor);
}
UWieldComponent* UEquipmentHelperLibrary::GetWieldPS(const AActor* FromActor)
{
	return GetPSComp<UWieldComponent>(FromActor);
}

// ---------- Assets ----------
UItemDataAsset* UEquipmentHelperLibrary::LoadItemDataByTag(FGameplayTag ItemIDTag, bool bSynchronousLoad)
{
	if (!ItemIDTag.IsValid()) return nullptr;
	return UInventoryAssetManager::Get().LoadItemDataByTag(ItemIDTag, bSynchronousLoad);
}

// ---------- Equipment ----------
bool UEquipmentHelperLibrary::EquipItemByTag(AActor* ContextActor, FGameplayTag SlotTag, FGameplayTag ItemIDTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->TryEquipByItemIDTag(SlotTag, ItemIDTag);
	}
	return false;
}

bool UEquipmentHelperLibrary::UnequipSlot(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->TryUnequipSlot(SlotTag);
	}
	return false;
}

UItemDataAsset* UEquipmentHelperLibrary::GetEquippedItemData(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
	{
		return E->GetEquippedItemData(SlotTag);
	}
	return nullptr;
}

// ---------- Wield ----------
bool UEquipmentHelperLibrary::WieldEquippedSlot(AActor* ContextActor, FGameplayTag SlotTag)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->TryWieldEquippedInSlot(SlotTag);
	}
	return false;
}

bool UEquipmentHelperLibrary::ToggleHolster(AActor* ContextActor)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->TryToggleHolster();
	}
	return false;
}

bool UEquipmentHelperLibrary::Holster(AActor* ContextActor)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->TryHolster();
	}
	return false;
}

bool UEquipmentHelperLibrary::Unholster(AActor* ContextActor)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->TryUnholster();
	}
	return false;
}

bool UEquipmentHelperLibrary::SetWieldProfile(AActor* ContextActor, FGameplayTag ProfileTag)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->TrySetActiveWieldProfile(ProfileTag);
	}
	return false;
}

bool UEquipmentHelperLibrary::IsHolstered(AActor* ContextActor)
{
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		return W->IsHolstered();
	}
	return true;
}

// ---------- Toolbar / Zones ----------
void UEquipmentHelperLibrary::ToolbarCycleNext(AActor* ContextActor)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		T->TryCycleNext();
	}
}

void UEquipmentHelperLibrary::ToolbarCyclePrev(AActor* ContextActor)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		T->TryCyclePrev();
	}
}

void UEquipmentHelperLibrary::NotifyZoneTags(AActor* ContextActor, const FGameplayTagContainer& ZoneTags)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		T->TryNotifyZoneTagsUpdated(ZoneTags);
	}
}

void UEquipmentHelperLibrary::SetCombatState(AActor* ContextActor, bool bInCombat)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		T->TrySetCombatState(bInCombat);
	}
	if (UWieldComponent* W = GetWieldPS(ContextActor))
	{
		W->TrySetCombatState(bInCombat);
	}
}

// Convenience reads
TArray<FGameplayTag> UEquipmentHelperLibrary::GetToolbarItemIDs(AActor* ContextActor)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		return T->ToolbarItemIDs;
	}
	return {};
}

int32 UEquipmentHelperLibrary::GetToolbarActiveIndex(AActor* ContextActor)
{
	if (UDynamicToolbarComponent* T = GetToolbarPS(ContextActor))
	{
		return T->ActiveIndex;
	}
	return INDEX_NONE;
}
