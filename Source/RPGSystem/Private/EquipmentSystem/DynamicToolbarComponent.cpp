// DynamicToolbarComponent.cpp

#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryItem.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

static bool MatchesItemQuery(const UItemDataAsset* Item, const FGameplayTagQuery& Query)
{
	if (!Item) return false;
	if (Query.IsEmpty()) return true;
	FGameplayTagContainer Owned;
	Owned.AddTag(Item->ItemIDTag);
	Owned.AddTag(Item->ItemType);
	Owned.AddTag(Item->ItemCategory);
	Owned.AddTag(Item->ItemSubCategory);
	return Query.Matches(Owned);
}

UDynamicToolbarComponent::UDynamicToolbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDynamicToolbarComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildForZone();
	UpdateVisibilityForZone();
}

void UDynamicToolbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDynamicToolbarComponent, ToolbarItemIDs);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveIndex);
}

void UDynamicToolbarComponent::OnRep_Toolbar()
{
	OnToolbarChanged.Broadcast();
	if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
	{
		UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ToolbarItemIDs[ActiveIndex], true);
		OnActiveToolChanged.Broadcast(ActiveIndex, Data);
	}
}

void UDynamicToolbarComponent::OnRep_ActiveIndex()
{
	if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
	{
		UItemDataAsset* Data = UInventoryAssetManager::Get().LoadItemDataByTag(ToolbarItemIDs[ActiveIndex], true);
		OnActiveToolChanged.Broadcast(ActiveIndex, Data);
	}
}

UInventoryComponent* UDynamicToolbarComponent::ResolveInventory() const
{
	if (AActor* O = GetOwner()) return O->FindComponentByClass<UInventoryComponent>();
	return nullptr;
}

UEquipmentComponent* UDynamicToolbarComponent::ResolveEquipment() const
{
	if (AActor* O = GetOwner()) return O->FindComponentByClass<UEquipmentComponent>();
	return nullptr;
}

void UDynamicToolbarComponent::NotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags)
{
	CurrentZoneTags = ZoneTags;
	if (!bManualOverride) RebuildForZone();
	UpdateVisibilityForZone();
}

void UDynamicToolbarComponent::SetCombatState(bool bNewInCombat)
{
	bInCombat = bNewInCombat;
	if (!bManualOverride) RebuildForZone();
}

void UDynamicToolbarComponent::SetManualOverride(bool bEnable)
{
	bManualOverride = bEnable;
}

void UDynamicToolbarComponent::CycleNext()
{
	if (ToolbarItemIDs.Num()==0) return;
	ActiveIndex = (ActiveIndex + 1) % ToolbarItemIDs.Num();
	OnRep_ActiveIndex();

	if (bAutoEquipActiveTool && !bInCombat)
	{
		if (UEquipmentComponent* Equip = ResolveEquipment())
		{
			if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
			{
				Equip->TryEquipByItemIDTag(ActiveToolSlotTag, ToolbarItemIDs[ActiveIndex]);
			}
		}
	}
}

void UDynamicToolbarComponent::CyclePrev()
{
	if (ToolbarItemIDs.Num()==0) return;
	ActiveIndex = (ActiveIndex - 1 + ToolbarItemIDs.Num()) % ToolbarItemIDs.Num();
	OnRep_ActiveIndex();

	if (bAutoEquipActiveTool && !bInCombat)
	{
		if (UEquipmentComponent* Equip = ResolveEquipment())
		{
			if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
			{
				Equip->TryEquipByItemIDTag(ActiveToolSlotTag, ToolbarItemIDs[ActiveIndex]);
			}
		}
	}
}

void UDynamicToolbarComponent::SetActiveToolIndex(int32 NewIndex)
{
	if (!ToolbarItemIDs.IsValidIndex(NewIndex)) return;
	ActiveIndex = NewIndex;
	OnRep_ActiveIndex();

	if (bAutoEquipActiveTool && !bInCombat)
	{
		if (UEquipmentComponent* Equip = ResolveEquipment())
		{
			if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
			{
				Equip->TryEquipByItemIDTag(ActiveToolSlotTag, ToolbarItemIDs[ActiveIndex]);
			}
		}
	}
}

void UDynamicToolbarComponent::RebuildForZone()
{
	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority()) return;
	}

	UInventoryComponent* Inv = ResolveInventory();
	if (!Inv) return;

	TArray<FGameplayTag> OutIDs;
	FGameplayTagQuery QueryToUse;

	if (bInCombat && !CombatAllowedQuery.IsEmpty())
	{
		QueryToUse = CombatAllowedQuery;
	}
	else
	{
		for (const TPair<FGameplayTag, FGameplayTagQuery>& Pair : ZoneAllowedQueries)
		{
			if (CurrentZoneTags.HasTagExact(Pair.Key))
			{
				QueryToUse = Pair.Value;
				break;
			}
		}
	}

	const TArray<FInventoryItem>& All = Inv->GetItems();
	for (const FInventoryItem& It : All)
	{
		if (!It.IsValid()) continue;
		if (UItemDataAsset* D = It.ItemData.Get())
		{
			if (MatchesItemQuery(D, QueryToUse))
			{
				OutIDs.Add(D->ItemIDTag);
				if (OutIDs.Num() >= NumSlots) break;
			}
		}
	}

	ToolbarItemIDs = OutIDs;
	OnRep_Toolbar();

	if (bAutoEquipActiveTool && ToolbarItemIDs.IsValidIndex(ActiveIndex) && !bInCombat)
	{
		if (UEquipmentComponent* Equip = ResolveEquipment())
		{
			Equip->TryEquipByItemIDTag(ActiveToolSlotTag, ToolbarItemIDs[ActiveIndex]);
		}
	}
}

void UDynamicToolbarComponent::UpdateVisibilityForZone()
{
	const bool bWasVisible = bToolbarVisible;
	if (bHideInBaseZone && BaseZoneTag.IsValid() && CurrentZoneTags.HasTagExact(BaseZoneTag))
	{
		bToolbarVisible = false;
	}
	else
	{
		bToolbarVisible = true;
	}
	if (bToolbarVisible != bWasVisible)
	{
		OnToolbarVisibilityChanged.Broadcast(bToolbarVisible);
	}
}

// Client wrappers
void UDynamicToolbarComponent::TryNotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { NotifyZoneTagsUpdated(ZoneTags); return; } ServerNotifyZoneTagsUpdated(ZoneTags); }
}
void UDynamicToolbarComponent::TrySetCombatState(bool bNewInCombat)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetCombatState(bNewInCombat); return; } ServerSetCombatState(bNewInCombat); }
}
void UDynamicToolbarComponent::TrySetManualOverride(bool bEnable)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetManualOverride(bEnable); return; } ServerSetManualOverride(bEnable); }
}
void UDynamicToolbarComponent::TryCycleNext()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { CycleNext(); return; } ServerCycleNext(); }
}
void UDynamicToolbarComponent::TryCyclePrev()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { CyclePrev(); return; } ServerCyclePrev(); }
}
void UDynamicToolbarComponent::TrySetActiveToolIndex(int32 NewIndex)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetActiveToolIndex(NewIndex); return; } ServerSetActiveToolIndex(NewIndex); }
}

// RPCs
bool UDynamicToolbarComponent::ServerNotifyZoneTagsUpdated_Validate(FGameplayTagContainer){ return true; }
void UDynamicToolbarComponent::ServerNotifyZoneTagsUpdated_Implementation(FGameplayTagContainer ZoneTags){ NotifyZoneTagsUpdated(ZoneTags); }

bool UDynamicToolbarComponent::ServerSetCombatState_Validate(bool){ return true; }
void UDynamicToolbarComponent::ServerSetCombatState_Implementation(bool bNewInCombat){ SetCombatState(bNewInCombat); }

bool UDynamicToolbarComponent::ServerSetManualOverride_Validate(bool){ return true; }
void UDynamicToolbarComponent::ServerSetManualOverride_Implementation(bool bEnable){ SetManualOverride(bEnable); }

bool UDynamicToolbarComponent::ServerCycleNext_Validate(){ return true; }
void UDynamicToolbarComponent::ServerCycleNext_Implementation(){ CycleNext(); }

bool UDynamicToolbarComponent::ServerCyclePrev_Validate(){ return true; }
void UDynamicToolbarComponent::ServerCyclePrev_Implementation(){ CyclePrev(); }

bool UDynamicToolbarComponent::ServerSetActiveToolIndex_Validate(int32){ return true; }
void UDynamicToolbarComponent::ServerSetActiveToolIndex_Implementation(int32 NewIndex){ SetActiveToolIndex(NewIndex); }
