// Source/RPGSystem/Private/EquipmentSystem/WieldComponent.cpp
// All rights Reserved Midnight Entertainment Studios LLC

#include "EquipmentSystem/WieldComponent.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/WeaponItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"

UWieldComponent::UWieldComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UWieldComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UEquipmentComponent* E = ResolveEquipment())
	{
		E->OnEquipmentChanged.AddDynamic(this, &UWieldComponent::HandleEquipmentChanged);
		E->OnEquipmentSlotCleared.AddDynamic(this, &UWieldComponent::HandleEquipmentCleared);
	}
	if (UDynamicToolbarComponent* T = ResolveToolbar())
	{
		T->OnActiveToolChanged.AddDynamic(this, &UWieldComponent::HandleToolbarActiveChanged);
	}

	ActiveWieldProfile = DefaultWieldProfile;
	ReevaluateCandidate();
}

void UWieldComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWieldComponent, WieldedItemIDTag);
	DOREPLIFETIME(UWieldComponent, bIsHolstered);
	DOREPLIFETIME(UWieldComponent, bInCombat);
	DOREPLIFETIME(UWieldComponent, ActiveWieldProfile);
}

UEquipmentComponent* UWieldComponent::ResolveEquipment() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UEquipmentComponent>() : nullptr;
}
UDynamicToolbarComponent* UWieldComponent::ResolveToolbar() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDynamicToolbarComponent>() : nullptr;
}

UItemDataAsset* UWieldComponent::LoadByItemTag(const FGameplayTag& ItemID) const
{
	return UInventoryHelpers::FindItemDataByTag(GetOwner(), ItemID);
}

UItemDataAsset* UWieldComponent::GetCurrentWieldedItemData() const
{
	if (!WieldedItemIDTag.IsValid()) return nullptr;
	return LoadByItemTag(WieldedItemIDTag);
}

void UWieldComponent::OnRep_WieldedItem()
{
	OnWieldedItemChanged.Broadcast(WieldedItemIDTag, GetCurrentWieldedItemData());
}
void UWieldComponent::OnRep_Holstered()
{
	OnHolsterStateChanged.Broadcast(bIsHolstered);
}
void UWieldComponent::OnRep_ActiveWieldProfile()
{
	OnPoseTagChanged.Broadcast(ResolvePoseTag(GetCurrentWieldedItemData()));
}

FSlotSocketDef UWieldComponent::GetSocketDefForSlot(const FGameplayTag& SlotTag) const
{
	for (const FSlotSocketDef& Def : SlotSockets)
	{
		if (Def.SlotTag.MatchesTagExact(SlotTag))
			return Def;
	}
	return FSlotSocketDef{};
}

void UWieldComponent::ResolveAttachSockets(UItemDataAsset* ItemData, const FGameplayTag& SlotTag, FName& OutHand, FName& OutHolster, FName& OutOffhandIK) const
{
	OutHand = NAME_None; OutHolster = NAME_None; OutOffhandIK = NAME_None;

	if (const UWeaponItemDataAsset* W = Cast<UWeaponItemDataAsset>(ItemData))
	{
		OutHand = W->EquippedSocket;
		OutHolster = W->HolsteredSocket;
	}

	if (OutHand.IsNone() || OutHolster.IsNone())
	{
		const FSlotSocketDef Def = GetSocketDefForSlot(SlotTag);
		if (OutHand.IsNone())   OutHand   = Def.HandSocket;
		if (OutHolster.IsNone())OutHolster= Def.HolsterSocket;
	}
}

FGameplayTag UWieldComponent::ResolvePoseTag(UItemDataAsset* ItemData) const
{
	if (!ItemData) return FGameplayTag();
	if (const UWeaponItemDataAsset* W = Cast<UWeaponItemDataAsset>(ItemData))
	{
		if (W->MovementSetTag.IsValid())
			return W->MovementSetTag;
	}
	if (PoseBySlot.Contains(WieldSourceSlotTag))
		return PoseBySlot[WieldSourceSlotTag];

	for (const TPair<FGameplayTag, FGameplayTag>& Pair : PoseByItemCategory)
	{
		if (ItemData->AdditionalTags.Contains(Pair.Key))
			return Pair.Value;
	}
	return FGameplayTag();
}

void UWieldComponent::ComputeAndBroadcastCandidate()
{
	if (bUseToolbarActiveAsFallback)
	{
		if (UDynamicToolbarComponent* T = ResolveToolbar())
		{
			FGameplayTag ActiveSlot;
			if (T->GetActiveSlotTag(ActiveSlot))
			{
				if (UEquipmentComponent* E = ResolveEquipment())
				{
					if (UItemDataAsset* D = E->GetEquippedItemData(ActiveSlot))
					{
						OnWieldCandidateChanged.Broadcast(ActiveSlot, D);
						return;
					}
				}
			}
		}
	}

	if (UEquipmentComponent* E = ResolveEquipment())
	{
		TArray<FGameplayTag> Pref;
		GetActivePreferredSlots(Pref);
		for (const FGameplayTag& Slot : Pref)
		{
			if (UItemDataAsset* D = E->GetEquippedItemData(Slot))
			{
				OnWieldCandidateChanged.Broadcast(Slot, D);
				return;
			}
		}
	}

	OnWieldCandidateChanged.Broadcast(FGameplayTag(), nullptr);
}

void UWieldComponent::GetActivePreferredSlots(TArray<FGameplayTag>& Out) const
{
	Out.Reset();
	if (!ActiveWieldProfile.IsValid() || !GetSlotsForProfile(ActiveWieldProfile, Out))
	{
		Out = PreferredWieldSlots;
	}
}

bool UWieldComponent::GetSlotsForProfile(const FGameplayTag& ProfileTag, TArray<FGameplayTag>& Out) const
{
	for (const FWieldProfileDef& P : WieldProfiles)
	{
		if (P.ProfileTag.MatchesTagExact(ProfileTag))
		{
			Out = P.SlotOrder;
			return true;
		}
	}
	return false;
}

// ---- actions (client entry) ----
bool UWieldComponent::TryToggleHolster()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerToggleHolster(); return false; }
	SetHolstered_Server(!bIsHolstered);
	return true;
}
bool UWieldComponent::TryHolster()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerSetHolstered(true); return false; }
	SetHolstered_Server(true);
	return true;
}
bool UWieldComponent::TryUnholster()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerSetHolstered(false); return false; }
	SetHolstered_Server(false);
	return true;
}
bool UWieldComponent::TryWieldEquippedInSlot(FGameplayTag SlotTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerWieldEquippedInSlot(SlotTag); return false; }
	WieldEquippedInSlot_Server(SlotTag);
	return true;
}
bool UWieldComponent::TrySetCombatState(bool bNewInCombat)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerSetCombatState(bNewInCombat); return false; }
	SetCombat_Server(bNewInCombat);
	return true;
}
void UWieldComponent::ReevaluateCandidate()
{
	ComputeAndBroadcastCandidate();
}
bool UWieldComponent::TrySetActiveWieldProfile(FGameplayTag ProfileTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { ServerSetActiveWieldProfile(ProfileTag); return false; }
	SetActiveWieldProfile_Server(ProfileTag);
	return true;
}

// ---- server-side impl ----
void UWieldComponent::SetHolstered_Server(bool bHolster)
{
	if (bIsHolstered == bHolster) return;
	bIsHolstered = bHolster;

	UItemDataAsset* Item = GetCurrentWieldedItemData();
	const FGameplayTag Pose = ResolvePoseTag(Item);
	OnHolsterStateChanged.Broadcast(bIsHolstered);
	OnPoseTagChanged.Broadcast(Pose);

	ApplyAbilitySetForItem(Item, /*bGive*/ !bIsHolstered);
}

void UWieldComponent::SetCombat_Server(bool bNewInCombat)
{
	bInCombat = bNewInCombat;
}

void UWieldComponent::WieldEquippedInSlot_Server(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid()) return;

	if (UEquipmentComponent* E = ResolveEquipment())
	{
		if (UItemDataAsset* D = E->GetEquippedItemData(SlotTag))
		{
			WieldSourceSlotTag = SlotTag;
			WieldedItemIDTag   = D->ItemIDTag;
			OnWieldedItemChanged.Broadcast(WieldedItemIDTag, D);

			if (bForceWeaponsInCombat || bIsHolstered)
				SetHolstered_Server(false);

			OnPoseTagChanged.Broadcast(ResolvePoseTag(D));
			return;
		}
	}
	WieldSourceSlotTag = FGameplayTag();
	WieldedItemIDTag   = FGameplayTag();
	OnWieldedItemChanged.Broadcast(WieldedItemIDTag, nullptr);
}

void UWieldComponent::ToggleHolster_Server()
{
	SetHolstered_Server(!bIsHolstered);
}

void UWieldComponent::SetActiveWieldProfile_Server(FGameplayTag ProfileTag)
{
	if (ActiveWieldProfile == ProfileTag) return;
	ActiveWieldProfile = ProfileTag;
	OnPoseTagChanged.Broadcast(ResolvePoseTag(GetCurrentWieldedItemData()));
	ReevaluateCandidate();
}

// ---- RPCs ----
bool UWieldComponent::ServerSetHolstered_Validate(bool) { return true; }
void UWieldComponent::ServerSetHolstered_Implementation(bool bHolster) { SetHolstered_Server(bHolster); }

bool UWieldComponent::ServerSetCombatState_Validate(bool) { return true; }
void UWieldComponent::ServerSetCombatState_Implementation(bool bNewInCombat) { SetCombat_Server(bNewInCombat); }

bool UWieldComponent::ServerWieldEquippedInSlot_Validate(FGameplayTag) { return true; }
void UWieldComponent::ServerWieldEquippedInSlot_Implementation(FGameplayTag SlotTag) { WieldEquippedInSlot_Server(SlotTag); }

bool UWieldComponent::ServerToggleHolster_Validate() { return true; }
void UWieldComponent::ServerToggleHolster_Implementation() { ToggleHolster_Server(); }

bool UWieldComponent::ServerSetActiveWieldProfile_Validate(FGameplayTag) { return true; }
void UWieldComponent::ServerSetActiveWieldProfile_Implementation(FGameplayTag ProfileTag) { SetActiveWieldProfile_Server(ProfileTag); }

// ---- event handlers ----
void UWieldComponent::HandleEquipmentChanged(FGameplayTag, UItemDataAsset*) { ReevaluateCandidate(); }
void UWieldComponent::HandleEquipmentCleared(FGameplayTag)                 { ReevaluateCandidate(); }
void UWieldComponent::HandleToolbarActiveChanged(int32, UItemDataAsset*)   { if (bUseToolbarActiveAsFallback) ReevaluateCandidate(); }

// ---- GAS/GSC bridge ----
UAbilitySystemComponent* UWieldComponent::ResolveASC_Engine() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
		return ASI->GetAbilitySystemComponent();
	return nullptr;
}

#if RPG_HAS_GSC
UGSCAbilitySystemComponent* UWieldComponent::ResolveASC_GSC() const
{
	return Cast<UGSCAbilitySystemComponent>(ResolveASC_Engine());
}
#endif

void UWieldComponent::ApplyAbilitySetForItem(UItemDataAsset* ItemData, bool bGive)
{
#if RPG_HAS_GSC
	UGSCAbilitySystemComponent* GSC = ResolveASC_GSC();
	if (!GSC || !ItemData) return;

	const FGameplayTag ItemID = ItemData->ItemIDTag;
	const TSoftObjectPtr<UGSCAbilitySet>* Found = AbilitySetByItemID.Find(ItemID);
	if (!Found || Found->IsNull()) return;

	UGSCAbilitySet* Set = Found->LoadSynchronous();
	if (!Set) return;

	if (bGive)
	{
		CurrentAbilitySetHandle = GSC->GiveAbilitySet(Set);
	}
	else
	{
		if (CurrentAbilitySetHandle.IsValid())
		{
			GSC->ClearAbilitySet(CurrentAbilitySetHandle);
		}
	}
#else
	(void)ItemData; (void)bGive;
#endif
}
