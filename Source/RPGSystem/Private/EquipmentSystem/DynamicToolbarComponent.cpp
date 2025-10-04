// All rights Reserved Midnight Entertainment Studios LLC
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "Net/UnrealNetwork.h"

UDynamicToolbarComponent::UDynamicToolbarComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UDynamicToolbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDynamicToolbarComponent, bVisible);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveIndex);
	DOREPLIFETIME(UDynamicToolbarComponent, ToolbarItemIDs);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveToolSlotTag);
	DOREPLIFETIME(UDynamicToolbarComponent, ActiveSetTags);
}

bool UDynamicToolbarComponent::IsAuth() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

UEquipmentComponent* UDynamicToolbarComponent::ResolveEquipment() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UEquipmentComponent>() : nullptr;
}

UWieldComponent* UDynamicToolbarComponent::ResolveWield() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UWieldComponent>() : nullptr;
}

void UDynamicToolbarComponent::OnRep_Visible()
{
	OnToolbarVisibilityChanged.Broadcast(bVisible);
}

void UDynamicToolbarComponent::OnRep_ActiveIndex()
{
	const FGameplayTag NewTag = (ActiveIndex >= 0 && ActiveIndex < ToolbarItemIDs.Num())
		? ToolbarItemIDs[ActiveIndex] : FGameplayTag();
	ActiveToolSlotTag = NewTag;
	OnActiveToolChanged.Broadcast(ActiveIndex, nullptr);
}

void UDynamicToolbarComponent::OnRep_ComposedSlots()
{
	ToolbarItemIDs = ComposedSlots;
	OnToolbarChanged.Broadcast();
}

void UDynamicToolbarComponent::RecomposeToolbar()
{
	ComposedSlots.Reset();

	// Compose order from active sets
	for (const FToolbarSlotSet& Set : SlotSets)
	{
		const bool bActive = ActiveSetTags.HasTagExact(Set.SetTag) || Set.bEnabledByDefault;
		if (!bActive) continue;

		for (const FGameplayTag& Tag : Set.SlotTags)
		{
			if (Tag.IsValid())
				ComposedSlots.Add(Tag);
		}
	}

	ToolbarItemIDs = ComposedSlots;

	// Clamp ActiveIndex and mirror ActiveToolSlotTag
	if (ComposedSlots.Num() == 0)
	{
		ActiveIndex = INDEX_NONE;
		ActiveToolSlotTag = FGameplayTag();
	}
	else
	{
		if (ActiveIndex < 0 || ActiveIndex >= ComposedSlots.Num())
			ActiveIndex = 0;
		ActiveToolSlotTag = ComposedSlots[ActiveIndex];
	}

	OnToolbarChanged.Broadcast();
	OnActiveToolChanged.Broadcast(ActiveIndex, nullptr);
}

void UDynamicToolbarComponent::ActivateSet(FGameplayTag SetTag)
{
	if (!IsAuth()) { ServerActivateSet(SetTag); return; }
	ActiveSetTags.AddTag(SetTag);
	RecomposeToolbar();
}
void UDynamicToolbarComponent::DeactivateSet(FGameplayTag SetTag)
{
	if (!IsAuth()) { ServerDeactivateSet(SetTag); return; }
	ActiveSetTags.RemoveTag(SetTag);
	RecomposeToolbar();
}
void UDynamicToolbarComponent::SetActiveSets(FGameplayTagContainer NewActiveSets)
{
	if (!IsAuth()) { ServerSetActiveSets(NewActiveSets); return; }
	ActiveSetTags = NewActiveSets;
	RecomposeToolbar();
}
void UDynamicToolbarComponent::SetActiveIndex(int32 NewIndex)
{
	if (!IsAuth()) { ServerSetActiveIndex(NewIndex); return; }
	ActiveIndex = NewIndex;
	const FGameplayTag NewTag = (ActiveIndex >= 0 && ActiveIndex < ToolbarItemIDs.Num())
		? ToolbarItemIDs[ActiveIndex] : FGameplayTag();
	ActiveToolSlotTag = NewTag;
	OnActiveToolChanged.Broadcast(ActiveIndex, nullptr);
}

void UDynamicToolbarComponent::CycleNext()
{
	if (!IsAuth()) { ServerSetActiveIndex(ActiveIndex + 1); return; }
	if (ComposedSlots.Num() <= 0) return;
	ActiveIndex = (ActiveIndex + 1 + ComposedSlots.Num()) % ComposedSlots.Num();
	ActiveToolSlotTag = ComposedSlots[ActiveIndex];
	OnActiveToolChanged.Broadcast(ActiveIndex, nullptr);
}
void UDynamicToolbarComponent::CyclePrev()
{
	if (!IsAuth()) { ServerSetActiveIndex(ActiveIndex - 1); return; }
	if (ComposedSlots.Num() <= 0) return;
	ActiveIndex = (ActiveIndex - 1 + ComposedSlots.Num()) % ComposedSlots.Num();
	ActiveToolSlotTag = ComposedSlots[ActiveIndex];
	OnActiveToolChanged.Broadcast(ActiveIndex, nullptr);
}

void UDynamicToolbarComponent::TryNotifyZoneTagsUpdated(const FGameplayTagContainer& /*ZoneTags*/) {}
void UDynamicToolbarComponent::TrySetCombatState(bool /*bInCombat*/) {}

void UDynamicToolbarComponent::WieldActiveSlot()
{
	if (UWieldComponent* W = ResolveWield())
	{
		if (ActiveToolSlotTag.IsValid())
			W->TryWieldEquippedInSlot(ActiveToolSlotTag);
	}
}

// ---------------- RPCs ----------------
void UDynamicToolbarComponent::ServerActivateSet_Implementation(FGameplayTag SetTag) { ActivateSet(SetTag); }
void UDynamicToolbarComponent::ServerDeactivateSet_Implementation(FGameplayTag SetTag) { DeactivateSet(SetTag); }
void UDynamicToolbarComponent::ServerSetActiveSets_Implementation(FGameplayTagContainer NewActiveSets) { SetActiveSets(NewActiveSets); }
void UDynamicToolbarComponent::ServerSetActiveIndex_Implementation(int32 NewIndex) { SetActiveIndex(NewIndex); }
void UDynamicToolbarComponent::ServerWieldActiveSlot_Implementation() { WieldActiveSlot(); }

// ---------------- Helper UFUNCTION bodies ----------------

bool UDynamicToolbarComponent::GetActiveSlotTag(FGameplayTag& OutSlotTag) const
{
	OutSlotTag = ActiveToolSlotTag;
	return ActiveToolSlotTag.IsValid();
}

static FText MakeNameFromTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
		return FText::FromString(TEXT("Unnamed"));
	return FText::FromName(Tag.GetTagName());
}

void UDynamicToolbarComponent::GetAllSetNames(TMap<FGameplayTag, FText>& Out) const
{
	Out.Reset();
	for (const FToolbarSlotSet& Set : SlotSets)
	{
		Out.Add(Set.SetTag, MakeNameFromTag(Set.SetTag));
	}
}

void UDynamicToolbarComponent::GetActiveSetNames(TMap<FGameplayTag, FText>& Out) const
{
	Out.Reset();
	for (const FToolbarSlotSet& Set : SlotSets)
	{
		if (ActiveSetTags.HasTagExact(Set.SetTag))
		{
			Out.Add(Set.SetTag, MakeNameFromTag(Set.SetTag));
		}
	}
}
