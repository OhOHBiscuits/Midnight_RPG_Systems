// EquipmentSystem/WieldComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"

// Engine ASC (always available)
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

// ---- Optional GSC support (compile-time detection) --------------------------
#if __has_include("Components/GSCAbilitySystemComponent.h")
  #include "Components/GSCAbilitySystemComponent.h"
  #if __has_include("Abilities/GSCAbilitySet.h")
    #include "Abilities/GSCAbilitySet.h"
  #else
    #include "GSCAbilitySet.h"
  #endif
  #define RPG_HAS_GSC 1
#else
  // Forward declare so TSoftObjectPtr<UGSCAbilitySet> is still a valid type, even if we won't load it.
  class UGSCAbilitySet;
  struct FGSCAbilitySetHandle { bool IsValid() const { return false; } };
  #define RPG_HAS_GSC 0
#endif
// -----------------------------------------------------------------------------

#include "WieldComponent.generated.h"

class UItemDataAsset;
class UEquipmentComponent;
class UDynamicToolbarComponent;

/** Pawn implements this to attach visuals & apply pose tags (BP-friendly). */
UINTERFACE(BlueprintType)
class RPGSYSTEM_API UEquipmentPresentationInterface : public UInterface
{
	GENERATED_BODY()
};

class RPGSYSTEM_API IEquipmentPresentationInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment|Presentation")
	void Equip_AttachInHands(UItemDataAsset* ItemData, FName HandSocket, FGameplayTag PoseTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment|Presentation")
	void Equip_AttachToHolster(UItemDataAsset* ItemData, FName HolsterSocket, FGameplayTag PoseTag);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment|Presentation")
	void Equip_ClearAttachedItem();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment|Presentation")
	void Equip_ApplyPoseTag(FGameplayTag PoseTag);
};

/** Optional per-slot socket fallback (only used when item gives none). */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FSlotSocketDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Sockets")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Sockets")
	FName HandSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Sockets")
	FName HolsterSocket = NAME_None;
};

/** Authorable wield profile: ordered preferred slots. */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FWieldProfileDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	FGameplayTag ProfileTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	TArray<FGameplayTag> SlotOrder;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHolsterStateChanged, bool, bIsHolstered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWieldedItemChanged, FGameplayTag, ItemIDTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWieldCandidateChanged, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoseTagChanged, FGameplayTag, PoseTag);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Weapon Manager (Wield)"))
class RPGSYSTEM_API UWieldComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWieldComponent();

	// -------- Config --------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	TArray<FGameplayTag> PreferredWieldSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield", meta=(TitleProperty="ProfileTag"))
	TArray<FWieldProfileDef> WieldProfiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	FGameplayTag DefaultWieldProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	bool bAutoChooseCandidate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	bool bForceWeaponsInCombat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield")
	bool bUseToolbarActiveAsFallback = true;

	/** Slot-based fallback sockets (only used when an item provides none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield|Sockets", meta=(TitleProperty="SlotTag"))
	TArray<FSlotSocketDef> SlotSockets;

	/** Optional: pose by item category; AnimBP reads the PoseTag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield|Anim")
	TMap<FGameplayTag, FGameplayTag> PoseByItemCategory;

	/** Optional: pose by slot (fallback if category not found). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield|Anim")
	TMap<FGameplayTag, FGameplayTag> PoseBySlot;

	/** ItemIDTag -> GSC Ability Set asset (granted on unholster, cleared on holster). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Wield|GAS", meta=(EditConditionHides, EditCondition="true"))
	TMap<FGameplayTag, TSoftObjectPtr<UGSCAbilitySet>> AbilitySetByItemID;

	// -------- State (replicated) --------
	UPROPERTY(ReplicatedUsing=OnRep_WieldedItem, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|Wield")
	FGameplayTag WieldedItemIDTag;

	UPROPERTY(ReplicatedUsing=OnRep_Holstered, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|Wield")
	bool bIsHolstered = true;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|Wield")
	bool bInCombat = false;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveWieldProfile, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|Wield")
	FGameplayTag ActiveWieldProfile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|Wield")
	FGameplayTag WieldSourceSlotTag;

	/** Handle for the currently granted GSC set (only meaningful when RPG_HAS_GSC==1). */
	FGSCAbilitySetHandle CurrentAbilitySetHandle;

	// -------- Events --------
	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Wield|Events")
	FOnHolsterStateChanged OnHolsterStateChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Wield|Events")
	FOnWieldedItemChanged OnWieldedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Wield|Events")
	FOnWieldCandidateChanged OnWieldCandidateChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Wield|Events")
	FOnPoseTagChanged OnPoseTagChanged;

	// -------- Queries --------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Wield|Queries")
	UItemDataAsset* GetCurrentWieldedItemData() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Wield|Queries")
	bool IsHolstered() const { return bIsHolstered; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Wield|Queries")
	FGameplayTag GetActiveWieldProfile() const { return ActiveWieldProfile; }

	// -------- Actions (server-auth; client-safe Try* wrappers) --------
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TryToggleHolster();

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TryHolster();

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TryUnholster();

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TryWieldEquippedInSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TrySetCombatState(bool bNewInCombat);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	void ReevaluateCandidate();

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Wield|Actions")
	bool TrySetActiveWieldProfile(FGameplayTag ProfileTag);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_WieldedItem();
	UFUNCTION() void OnRep_Holstered();
	UFUNCTION() void OnRep_ActiveWieldProfile();

	UEquipmentComponent* ResolveEquipment() const;
	UDynamicToolbarComponent* ResolveToolbar() const;
	UItemDataAsset* LoadByItemTag(const FGameplayTag& ItemID) const;

	void GetActivePreferredSlots(TArray<FGameplayTag>& Out) const;
	bool GetSlotsForProfile(const FGameplayTag& ProfileTag, TArray<FGameplayTag>& Out) const;
	void ComputeAndBroadcastCandidate();

	// Sockets / pose helpers (prefer per-item, then fallback by slot)
	void ResolveAttachSockets(UItemDataAsset* ItemData, const FGameplayTag& SlotTag, FName& OutHand, FName& OutHolster, FName& OutOffhandIK) const;
	FSlotSocketDef GetSocketDefForSlot(const FGameplayTag& SlotTag) const;
	FGameplayTag   ResolvePoseTag(UItemDataAsset* ItemData) const;

	// Server impls
	void SetHolstered_Server(bool bHolster);
	void SetCombat_Server(bool bNewInCombat);
	void WieldEquippedInSlot_Server(FGameplayTag SlotTag);
	void ToggleHolster_Server();
	void SetActiveWieldProfile_Server(FGameplayTag ProfileTag);

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetHolstered(bool bHolster);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetCombatState(bool bNewInCombat);
	UFUNCTION(Server, Reliable, WithValidation) void ServerWieldEquippedInSlot(FGameplayTag SlotTag);
	UFUNCTION(Server, Reliable, WithValidation) void ServerToggleHolster();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetActiveWieldProfile(FGameplayTag ProfileTag);

	// Bound handlers
	UFUNCTION() void HandleEquipmentChanged(FGameplayTag SlotTag, UItemDataAsset* ItemData);
	UFUNCTION() void HandleEquipmentCleared(FGameplayTag SlotTag);
	UFUNCTION() void HandleToolbarActiveChanged(int32 NewIndex, UItemDataAsset* ItemData);

	// GAS glue (engine) + optional GSC glue
	UAbilitySystemComponent* ResolveASC_Engine() const;
#if RPG_HAS_GSC
	UGSCAbilitySystemComponent* ResolveASC_GSC() const;
#endif
	void ApplyAbilitySetForItem(UItemDataAsset* ItemData, bool bGive);
};
