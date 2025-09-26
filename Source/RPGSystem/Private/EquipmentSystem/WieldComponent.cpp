// EquipmentSystem/WieldComponent.cpp

#include "EquipmentSystem/WieldComponent.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/WeaponItemDataAsset.h"
#include "Inventory/RangedWeaponItemDataAsset.h"
#include "EquipmentSystem/EquipmentSystemInterface.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

UWieldComponent::UWieldComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	PreferredWieldSlots = {
		FGameplayTag::RequestGameplayTag(TEXT("Slots.Weapon.Primary"),   false),
		FGameplayTag::RequestGameplayTag(TEXT("Slots.Weapon.Secondary"), false),
		FGameplayTag::RequestGameplayTag(TEXT("Slots.Weapon.SideArm"),   false),
		FGameplayTag::RequestGameplayTag(TEXT("Slots.Weapon.Melee"),     false),
		FGameplayTag::RequestGameplayTag(TEXT("Slots.Tool.Active"),      false)
	};

	DefaultWieldProfile = FGameplayTag::RequestGameplayTag(TEXT("WieldProfile.Default"), false);
}

void UWieldComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ActiveWieldProfile.IsValid() && DefaultWieldProfile.IsValid())
	{
		ActiveWieldProfile = DefaultWieldProfile;
	}

	if (UEquipmentComponent* Equip = ResolveEquipment())
	{
		Equip->OnEquippedItemChanged.AddDynamic(this, &UWieldComponent::HandleEquipmentChanged);
		Equip->OnEquippedSlotCleared.AddDynamic(this, &UWieldComponent::HandleEquipmentCleared);
	}
	if (UDynamicToolbarComponent* Toolbar = ResolveToolbar())
	{
		Toolbar->OnActiveToolChanged.AddDynamic(this, &UWieldComponent::HandleToolbarActiveChanged);
	}

	if (bAutoChooseCandidate)
	{
		ComputeAndBroadcastCandidate();
	}
}

void UWieldComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWieldComponent, WieldedItemIDTag);
	DOREPLIFETIME(UWieldComponent, bIsHolstered);
	DOREPLIFETIME(UWieldComponent, bInCombat);
	DOREPLIFETIME(UWieldComponent, ActiveWieldProfile);
}

// -------- Rep Notifies --------

void UWieldComponent::OnRep_WieldedItem()
{
	UItemDataAsset* Data = LoadByItemTag(WieldedItemIDTag);
	OnWieldedItemChanged.Broadcast(WieldedItemIDTag, Data);

	const FGameplayTag PoseTag = ResolvePoseTag(Data);
	OnPoseTagChanged.Broadcast(PoseTag);

	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->GetClass()->ImplementsInterface(UEquipmentPresentationInterface::StaticClass()))
		{
			FName HandSocket = NAME_None, HolsterSocket = NAME_None, OffhandIK = NAME_None;
			ResolveAttachSockets(Data, WieldSourceSlotTag, HandSocket, HolsterSocket, OffhandIK);

			if (!bIsHolstered)
				IEquipmentPresentationInterface::Execute_Equip_AttachInHands(P, Data, HandSocket, PoseTag);
			else
				IEquipmentPresentationInterface::Execute_Equip_AttachToHolster(P, Data, HolsterSocket, PoseTag);

			IEquipmentPresentationInterface::Execute_Equip_ApplyPoseTag(P, PoseTag);
		}
	}

	ApplyAbilitySetForItem(Data, !bIsHolstered);
}

void UWieldComponent::OnRep_Holstered()
{
	UItemDataAsset* Data = LoadByItemTag(WieldedItemIDTag);
	OnHolsterStateChanged.Broadcast(bIsHolstered);

	const FGameplayTag PoseTag = ResolvePoseTag(Data);
	OnPoseTagChanged.Broadcast(PoseTag);

	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->GetClass()->ImplementsInterface(UEquipmentPresentationInterface::StaticClass()))
		{
			FName HandSocket = NAME_None, HolsterSocket = NAME_None, OffhandIK = NAME_None;
			ResolveAttachSockets(Data, WieldSourceSlotTag, HandSocket, HolsterSocket, OffhandIK);

			if (bIsHolstered)
				IEquipmentPresentationInterface::Execute_Equip_AttachToHolster(P, Data, HolsterSocket, PoseTag);
			else
				IEquipmentPresentationInterface::Execute_Equip_AttachInHands(P, Data, HandSocket, PoseTag);

			IEquipmentPresentationInterface::Execute_Equip_ApplyPoseTag(P, PoseTag);
		}
	}

	ApplyAbilitySetForItem(Data, !bIsHolstered);
}

void UWieldComponent::OnRep_ActiveWieldProfile()
{
	if (bAutoChooseCandidate)
	{
		ComputeAndBroadcastCandidate();
	}
}

// -------- Helpers --------

UEquipmentComponent* UWieldComponent::ResolveEquipment() const
{
	if (AActor* O = GetOwner()) return O->FindComponentByClass<UEquipmentComponent>();
	return nullptr;
}
UDynamicToolbarComponent* UWieldComponent::ResolveToolbar() const
{
	if (AActor* O = GetOwner()) return O->FindComponentByClass<UDynamicToolbarComponent>();
	return nullptr;
}
UItemDataAsset* UWieldComponent::LoadByItemTag(const FGameplayTag& ItemID) const
{
	if (!ItemID.IsValid()) return nullptr;
	return UInventoryAssetManager::Get().LoadItemDataByTag(ItemID, /*bSynchronous*/ true);
}
UItemDataAsset* UWieldComponent::GetCurrentWieldedItemData() const
{
	return LoadByItemTag(WieldedItemIDTag);
}

bool UWieldComponent::GetSlotsForProfile(const FGameplayTag& ProfileTag, TArray<FGameplayTag>& Out) const
{
	for (const FWieldProfileDef& Def : WieldProfiles)
	{
		if (Def.ProfileTag == ProfileTag)
		{
			Out = Def.SlotOrder;
			return Out.Num() > 0;
		}
	}
	return false;
}

void UWieldComponent::GetActivePreferredSlots(TArray<FGameplayTag>& Out) const
{
	Out.Reset();
	if (ActiveWieldProfile.IsValid() && GetSlotsForProfile(ActiveWieldProfile, Out))
		return;

	Out = PreferredWieldSlots;
}

FSlotSocketDef UWieldComponent::GetSocketDefForSlot(const FGameplayTag& SlotTag) const
{
	for (const FSlotSocketDef& Def : SlotSockets)
	{
		if (Def.SlotTag == SlotTag)
			return Def;
	}
	return FSlotSocketDef{};
}

static FName ReadFNamePropOrNone(UObject* Obj, const TCHAR* PropName)
{
	if (!Obj) return NAME_None;
	if (FProperty* P = Obj->GetClass()->FindPropertyByName(PropName))
	{
		if (FNameProperty* NP = CastField<FNameProperty>(P))
			return *NP->ContainerPtrToValuePtr<FName>(Obj);
	}
	return NAME_None;
}

void UWieldComponent::ResolveAttachSockets(UItemDataAsset* ItemData, const FGameplayTag& SlotTag, FName& OutHand, FName& OutHolster, FName& OutOffhandIK) const
{
	OutHand = OutHolster = OutOffhandIK = NAME_None;

	if (const UWeaponItemDataAsset* Weapon = Cast<UWeaponItemDataAsset>(ItemData))
	{
		if (Weapon->EquippedSocket != NAME_None)  OutHand = Weapon->EquippedSocket;
		if (Weapon->HolsteredSocket != NAME_None) OutHolster = Weapon->HolsteredSocket;

		if (const URangedWeaponItemDataAsset* Ranged = Cast<URangedWeaponItemDataAsset>(ItemData))
			OutOffhandIK = Ranged->OffhandIKSocket;
	}

	if (OutHand == NAME_None)    OutHand    = ReadFNamePropOrNone(ItemData, TEXT("EquippedSocket"));
	if (OutHolster == NAME_None) OutHolster = ReadFNamePropOrNone(ItemData, TEXT("HolsteredSocket"));

	if (OutHand == NAME_None || OutHolster == NAME_None)
	{
		const FSlotSocketDef Fallback = GetSocketDefForSlot(SlotTag);
		if (OutHand == NAME_None)    OutHand    = Fallback.HandSocket;
		if (OutHolster == NAME_None) OutHolster = Fallback.HolsterSocket;
	}
}

FGameplayTag UWieldComponent::ResolvePoseTag(UItemDataAsset* ItemData) const
{
	if (ItemData)
	{
		static const FName PropName(TEXT("ItemCategory"));
		if (FProperty* Prop = ItemData->GetClass()->FindPropertyByName(PropName))
		{
			if (FStructProperty* SProp = CastField<FStructProperty>(Prop))
			{
				if (SProp->Struct == TBaseStructure<FGameplayTag>::Get())
				{
					const void* ValPtr = SProp->ContainerPtrToValuePtr<void>(ItemData);
					const FGameplayTag Category = *static_cast<const FGameplayTag*>(ValPtr);
					if (Category.IsValid())
					{
						if (const FGameplayTag* Found = PoseByItemCategory.Find(Category))
							return *Found;
					}
				}
			}
		}
	}

	if (const FGameplayTag* FoundBySlot = PoseBySlot.Find(WieldSourceSlotTag))
		return *FoundBySlot;

	return FGameplayTag();
}

void UWieldComponent::ComputeAndBroadcastCandidate()
{
	TArray<FGameplayTag> Order;
	GetActivePreferredSlots(Order);

	if (UEquipmentComponent* Equip = ResolveEquipment())
	{
		for (const FGameplayTag& SlotTag : Order)
		{
			if (!SlotTag.IsValid()) continue;
			if (UItemDataAsset* Data = Equip->GetEquippedItemData(SlotTag))
			{
				WieldSourceSlotTag = SlotTag;
				OnWieldCandidateChanged.Broadcast(SlotTag, Data);
				return;
			}
		}
	}

	if (!bInCombat && bUseToolbarActiveAsFallback)
	{
		if (UDynamicToolbarComponent* Toolbar = ResolveToolbar())
		{
			if (Toolbar->ToolbarItemIDs.IsValidIndex(Toolbar->ActiveIndex))
			{
				const FGameplayTag Candidate = Toolbar->ToolbarItemIDs[Toolbar->ActiveIndex];
				WieldSourceSlotTag = Toolbar->ActiveToolSlotTag;
				OnWieldCandidateChanged.Broadcast(WieldSourceSlotTag, LoadByItemTag(Candidate));
				return;
			}
		}
	}

	WieldSourceSlotTag = FGameplayTag();
	OnWieldCandidateChanged.Broadcast(FGameplayTag(), nullptr);
}

// -------- Actions (client wrappers) --------
bool UWieldComponent::TryToggleHolster()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { ToggleHolster_Server(); return true; } ServerToggleHolster(); return true; }
	return false;
}
bool UWieldComponent::TryHolster()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetHolstered_Server(true); return true; } ServerSetHolstered(true); return true; }
	return false;
}
bool UWieldComponent::TryUnholster()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetHolstered_Server(false); return true; } ServerSetHolstered(false); return true; }
	return false;
}
bool UWieldComponent::TryWieldEquippedInSlot(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid()) return false;
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { WieldEquippedInSlot_Server(SlotTag); return true; } ServerWieldEquippedInSlot(SlotTag); return true; }
	return false;
}
bool UWieldComponent::TrySetCombatState(bool bNewInCombat)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetCombat_Server(bNewInCombat); return true; } ServerSetCombatState(bNewInCombat); return true; }
	return false;
}
void UWieldComponent::ReevaluateCandidate()
{
	ComputeAndBroadcastCandidate();
}
bool UWieldComponent::TrySetActiveWieldProfile(FGameplayTag ProfileTag)
{
	if (!ProfileTag.IsValid()) return false;
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) { SetActiveWieldProfile_Server(ProfileTag); return true; } ServerSetActiveWieldProfile(ProfileTag); return true; }
	return false;
}

// -------- Server impls --------
void UWieldComponent::SetHolstered_Server(bool bHolster)
{
	bIsHolstered = bHolster;
	OnRep_Holstered();
}
void UWieldComponent::SetCombat_Server(bool bNewInCombat)
{
	bInCombat = bNewInCombat;
	if (bAutoChooseCandidate) ComputeAndBroadcastCandidate();
}
void UWieldComponent::WieldEquippedInSlot_Server(FGameplayTag SlotTag)
{
	UItemDataAsset* Data = nullptr;
	if (UEquipmentComponent* Equip = ResolveEquipment()) { Data = Equip->GetEquippedItemData(SlotTag); }
	if (!Data) return;

	WieldSourceSlotTag = SlotTag;
	WieldedItemIDTag = Data->ItemIDTag;
	OnRep_WieldedItem();

	if (bIsHolstered) { bIsHolstered = false; OnRep_Holstered(); }
}
void UWieldComponent::ToggleHolster_Server()
{
	SetHolstered_Server(!bIsHolstered);
}
void UWieldComponent::SetActiveWieldProfile_Server(FGameplayTag ProfileTag)
{
	TArray<FGameplayTag> Tmp;
	if (ProfileTag == DefaultWieldProfile || GetSlotsForProfile(ProfileTag, Tmp))
	{
		ActiveWieldProfile = ProfileTag;
		OnRep_ActiveWieldProfile();
	}
}

// -------- RPCs --------
bool UWieldComponent::ServerSetHolstered_Validate(bool){ return true; }
void UWieldComponent::ServerSetHolstered_Implementation(bool bHolster){ SetHolstered_Server(bHolster); }

bool UWieldComponent::ServerSetCombatState_Validate(bool){ return true; }
void UWieldComponent::ServerSetCombatState_Implementation(bool bNewInCombat){ SetCombat_Server(bNewInCombat); }

bool UWieldComponent::ServerWieldEquippedInSlot_Validate(FGameplayTag){ return true; }
void UWieldComponent::ServerWieldEquippedInSlot_Implementation(FGameplayTag SlotTag){ WieldEquippedInSlot_Server(SlotTag); }

bool UWieldComponent::ServerToggleHolster_Validate(){ return true; }
void UWieldComponent::ServerToggleHolster_Implementation(){ ToggleHolster_Server(); }

bool UWieldComponent::ServerSetActiveWieldProfile_Validate(FGameplayTag){ return true; }
void UWieldComponent::ServerSetActiveWieldProfile_Implementation(FGameplayTag ProfileTag){ SetActiveWieldProfile_Server(ProfileTag); }

// -------- Bound handlers --------
void UWieldComponent::HandleEquipmentChanged(FGameplayTag /*SlotTag*/, UItemDataAsset* /*ItemData*/)
{
	if (bAutoChooseCandidate) ComputeAndBroadcastCandidate();
}
void UWieldComponent::HandleEquipmentCleared(FGameplayTag /*SlotTag*/)
{
	if (bAutoChooseCandidate) ComputeAndBroadcastCandidate();
}
void UWieldComponent::HandleToolbarActiveChanged(int32 /*NewIndex*/, UItemDataAsset* /*ItemData*/)
{
	if (bAutoChooseCandidate && !bInCombat) ComputeAndBroadcastCandidate();
}

// -------- GAS glue (engine) + optional GSC glue --------
UAbilitySystemComponent* UWieldComponent::ResolveASC_Engine() const
{
	if (AActor* O = GetOwner())
	{
		// If component is on PlayerState (your setup), the PlayerState will implement UAbilitySystemInterface.
		if (const IAbilitySystemInterface* IFace = Cast<IAbilitySystemInterface>(O))
			return IFace->GetAbilitySystemComponent();

		// If on Pawn, climb to PS
		if (const APawn* P = Cast<APawn>(O))
		{
			if (APlayerState* PS = P->GetPlayerState())
			{
				if (const IAbilitySystemInterface* IFacePS = Cast<IAbilitySystemInterface>(PS))
					return IFacePS->GetAbilitySystemComponent();
			}
		}
	}
	return nullptr;
}

#if RPG_HAS_GSC
UGSCAbilitySystemComponent* UWieldComponent::ResolveASC_GSC() const
{
	// Prefer a real GSC ASC if present
	if (AActor* O = GetOwner())
	{
		if (UGSCAbilitySystemComponent* Direct = O->FindComponentByClass<UGSCAbilitySystemComponent>())
			return Direct;

		if (const APawn* P = Cast<APawn>(O))
		{
			if (APlayerState* PS = P->GetPlayerState())
				return PS->FindComponentByClass<UGSCAbilitySystemComponent>();
		}
	}
	return nullptr;
}
#endif

void UWieldComponent::ApplyAbilitySetForItem(UItemDataAsset* ItemData, bool bGive)
{
#if RPG_HAS_GSC
	if (!ItemData) return;

	UGSCAbilitySystemComponent* GSCASC = ResolveASC_GSC();
	if (!GSCASC) return;

	const TSoftObjectPtr<UGSCAbilitySet>* FoundPtr = AbilitySetByItemID.Find(ItemData->ItemIDTag);
	if (!FoundPtr || !FoundPtr->IsValid()) return;

	UGSCAbilitySet* Set = FoundPtr->LoadSynchronous();
	if (!Set) return;

	if (bGive)
	{
		FGSCAbilitySetHandle Handle;
		GSCASC->GiveAbilitySet(Set, Handle);
		CurrentAbilitySetHandle = Handle;
	}
	else
	{
		if (CurrentAbilitySetHandle.IsValid())
		{
			GSCASC->ClearAbilitySet(CurrentAbilitySetHandle);
			CurrentAbilitySetHandle = FGSCAbilitySetHandle();
		}
	}
#else
	// No GSC in compile scope — nothing to do (engine-only path).
	// You still have a valid build; when you wire Build.cs to include GASCompanion,
	// this code path automatically switches to the GSC grants above.
	(void)ItemData; (void)bGive;
#endif
}
