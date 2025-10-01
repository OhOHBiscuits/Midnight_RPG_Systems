#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "Net/UnrealNetwork.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "Inventory/ItemDataAsset.h"

UDynamicToolbarComponent::UDynamicToolbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDynamicToolbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDynamicToolbarComponent, bVisible);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveIndex);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveSetTags);
	// We replicate the mirror (ToolbarItemIDs) for convenience
	DOREPLIFETIME(UDynamicToolbarComponent, ToolbarItemIDs);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveToolSlotTag);
}

bool UDynamicToolbarComponent::IsAuth() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

UEquipmentComponent* UDynamicToolbarComponent::ResolveEquipment() const
{
	return UEquipmentHelperLibrary::GetEquipmentPS(GetOwner());
}

UWieldComponent* UDynamicToolbarComponent::ResolveWield() const
{
	return UEquipmentHelperLibrary::GetWieldPS(GetOwner());
}

void UDynamicToolbarComponent::OnRep_Visible()
{
	OnToolbarVisibilityChanged.Broadcast(bVisible);
}

void UDynamicToolbarComponent::OnRep_ComposedSlots()
{
	// ComposedSlots is not replicated directly; ToolbarItemIDs is.
	// Keep ActiveToolSlotTag consistent on clients when list changes.
	if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
	{
		ActiveToolSlotTag = ToolbarItemIDs[ActiveIndex];
	}
	else
	{
		ActiveToolSlotTag = FGameplayTag();
		ActiveIndex = INDEX_NONE;
	}

	OnToolbarChanged.Broadcast();

	// Let UI know the active tool's data (if any)
	if (ActiveIndex != INDEX_NONE)
	{
		UItemDataAsset* Data = UEquipmentHelperLibrary::GetEquippedItemData(GetOwner(), ActiveToolSlotTag);
		OnActiveToolChanged.Broadcast(ActiveIndex, Data);
	}
	else
	{
		OnActiveToolChanged.Broadcast(INDEX_NONE, nullptr);
	}
}

void UDynamicToolbarComponent::OnRep_ActiveIndex()
{
	if (ToolbarItemIDs.IsValidIndex(ActiveIndex))
	{
		ActiveToolSlotTag = ToolbarItemIDs[ActiveIndex];

		UItemDataAsset* Data = UEquipmentHelperLibrary::GetEquippedItemData(GetOwner(), ActiveToolSlotTag);
		OnActiveToolChanged.Broadcast(ActiveIndex, Data);
	}
	else
	{
		ActiveToolSlotTag = FGameplayTag();
		OnActiveToolChanged.Broadcast(INDEX_NONE, nullptr);
	}
}

void UDynamicToolbarComponent::GetAllSetNames(TMap<FGameplayTag, FText>& OutNames) const
{
	OutNames.Reset();
	for (const FToolbarSlotSet& S : SlotSets)
	{
		OutNames.Add(S.SetTag, S.DisplayName);
	}
}

void UDynamicToolbarComponent::GetActiveSetNames(TMap<FGameplayTag, FText>& OutNames) const
{
	OutNames.Reset();
	for (const FToolbarSlotSet& S : SlotSets)
	{
		if (ActiveSetTags.HasTag(S.SetTag))
		{
			OutNames.Add(S.SetTag, S.DisplayName);
		}
	}
}

bool UDynamicToolbarComponent::GetActiveSlotTag(FGameplayTag& OutSlot) const
{
	OutSlot = ActiveToolSlotTag;
	return ActiveToolSlotTag.IsValid();
}

// ---- composition ----

void UDynamicToolbarComponent::RecomposeToolbar()
{
	if (!IsAuth()) return;

	TArray<FGameplayTag> NewSlots;
	TSet<FGameplayTag> Seen;

	for (const FToolbarSlotSet& S : SlotSets)
	{
		if (!ActiveSetTags.HasTag(S.SetTag)) continue;

		for (const FGameplayTag& T : S.SlotTags)
		{
			if (T.IsValid() && !Seen.Contains(T))
			{
				Seen.Add(T);
				NewSlots.Add(T);
			}
		}
	}

	ComposedSlots = MoveTemp(NewSlots);
	ToolbarItemIDs = ComposedSlots; // mirror for BP/back-compat

	// Clamp and update ActiveToolSlotTag
	if (ToolbarItemIDs.Num() == 0)
	{
		ActiveIndex = INDEX_NONE;
		ActiveToolSlotTag = FGameplayTag();
	}
	else
	{
		if (!ToolbarItemIDs.IsValidIndex(ActiveIndex))
		{
			ActiveIndex = 0;
		}
		ActiveToolSlotTag = ToolbarItemIDs[ActiveIndex];
	}

	// Locally notify; replicated props will trigger client notifies
	OnRep_ComposedSlots();
}

// ---- control ----

void UDynamicToolbarComponent::ActivateSet(FGameplayTag SetTag)
{
	if (!SetTag.IsValid()) return;
	if (!IsAuth()) { ServerActivateSet(SetTag); return; }

	if (!ActiveSetTags.HasTag(SetTag))
	{
		ActiveSetTags.AddTag(SetTag);
		RecomposeToolbar();
	}
}

void UDynamicToolbarComponent::DeactivateSet(FGameplayTag SetTag)
{
	if (!SetTag.IsValid()) return;
	if (!IsAuth()) { ServerDeactivateSet(SetTag); return; }

	if (ActiveSetTags.HasTag(SetTag))
	{
		ActiveSetTags.RemoveTag(SetTag);
		RecomposeToolbar();
	}
}

void UDynamicToolbarComponent::SetActiveSets(FGameplayTagContainer NewActiveSets)
{
	if (!IsAuth()) { ServerSetActiveSets(NewActiveSets); return; }
	ActiveSetTags = MoveTemp(NewActiveSets);
	RecomposeToolbar();
}

void UDynamicToolbarComponent::SetActiveIndex(int32 NewIndex)
{
	if (!IsAuth()) { ServerSetActiveIndex(NewIndex); return; }
	ServerSetActiveIndex_Implementation(NewIndex);
}

void UDynamicToolbarComponent::CycleNext()
{
	SetActiveIndex(ActiveIndex + 1);
}

void UDynamicToolbarComponent::CyclePrev()
{
	SetActiveIndex(ActiveIndex - 1);
}

void UDynamicToolbarComponent::TryNotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags)
{
	// Lightweight default: if any set's SetTag exists in ZoneTags → activate it, else leave as-is.
	// You can make this stricter later or drive via a data table.
	if (!IsAuth()) { ServerSetActiveSets(ActiveSetTags); return; }

	bool bChanged = false;
	for (const FToolbarSlotSet& S : SlotSets)
	{
		const bool bShouldBeActive   = ZoneTags.HasTag(S.SetTag) || S.bEnabledByDefault;
		const bool bCurrentlyActive  = ActiveSetTags.HasTag(S.SetTag); // ← renamed (was bIsActive)

		if (bShouldBeActive != bCurrentlyActive)
		{
			bChanged = true;
			if (bShouldBeActive) { ActiveSetTags.AddTag(S.SetTag); }
			else                 { ActiveSetTags.RemoveTag(S.SetTag); }
		}
	}
	if (bChanged) RecomposeToolbar();
}
void UDynamicToolbarComponent::TrySetCombatState(bool bInCombat)
{
	if (!CombatSetTag.IsValid()) return; // nothing configured
	if (bInCombat) ActivateSet(CombatSetTag);
	else           DeactivateSet(CombatSetTag);
}

void UDynamicToolbarComponent::WieldActiveSlot()
{
	if (!IsAuth()) { ServerWieldActiveSlot(); return; }
	ServerWieldActiveSlot_Implementation();
}

// ---- RPCs ----

void UDynamicToolbarComponent::ServerActivateSet_Implementation(FGameplayTag SetTag)        { ActivateSet(SetTag); }
void UDynamicToolbarComponent::ServerDeactivateSet_Implementation(FGameplayTag SetTag)      { DeactivateSet(SetTag); }
void UDynamicToolbarComponent::ServerSetActiveSets_Implementation(FGameplayTagContainer S)  { ActiveSetTags = MoveTemp(S); RecomposeToolbar(); }

void UDynamicToolbarComponent::ServerSetActiveIndex_Implementation(int32 NewIndex)
{
	if (ToolbarItemIDs.Num() <= 0)
	{
		ActiveIndex = INDEX_NONE;
		ActiveToolSlotTag = FGameplayTag();
		OnRep_ActiveIndex();
		return;
	}

	const int32 Count = ToolbarItemIDs.Num();
	int32 Wrapped = (Count > 0) ? (NewIndex % Count) : INDEX_NONE;
	if (Wrapped < 0) Wrapped += Count;

	ActiveIndex = Wrapped;
	ActiveToolSlotTag = ToolbarItemIDs.IsValidIndex(ActiveIndex) ? ToolbarItemIDs[ActiveIndex] : FGameplayTag();
	OnRep_ActiveIndex();
}

void UDynamicToolbarComponent::ServerWieldActiveSlot_Implementation()
{
	if (!ToolbarItemIDs.IsValidIndex(ActiveIndex)) return;
	if (UWieldComponent* W = ResolveWield())
	{
		W->TryWieldEquippedInSlot(ToolbarItemIDs[ActiveIndex]);
	}
}
