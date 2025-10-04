#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DynamicToolbarComponent.generated.h"

class UEquipmentComponent;
class UWieldComponent;
class UItemDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToolbarChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolbarVisibilityChanged, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveToolChanged, int32, NewIndex, UItemDataAsset*, ItemData);

/** One named toolbar profile/set (e.g., Toolbar.Fishing) */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FToolbarSlotSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toolbar")
	FGameplayTag SetTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toolbar")
	FText DisplayName;

	/** Which family these slots belong to (e.g., Slots.Tool / Slots.Weapon / Slots.Armor) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toolbar")
	FGameplayTag AcceptRootTag;

	/** Concrete slot tags shown for this set (order matters) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toolbar")
	TArray<FGameplayTag> SlotTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Toolbar")
	bool bEnabledByDefault = false;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UDynamicToolbarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDynamicToolbarComponent();

	/** Author your sets here (Farming, Fishing, Combat, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Toolbar|Authoring")
	TArray<FToolbarSlotSet> SlotSets;

	/** Replicated: is the toolbar visible */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Visible, Category="1_Toolbar|State")
	bool bVisible = true;

	/** Replicated: active index into composed slots */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ActiveIndex, Category="1_Toolbar|State")
	int32 ActiveIndex = INDEX_NONE;

	/** Back-compat + easy BP access: mirror of composed slots (replicated) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ComposedSlots, Category="1_Toolbar|State")
	TArray<FGameplayTag> ToolbarItemIDs;

	/** Back-compat: the currently active slot tag (mirrors ActiveIndex) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ActiveIndex, Category="1_Toolbar|State")
	FGameplayTag ActiveToolSlotTag;

	/** Optional: which SetTag should represent "combat mode" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Authoring")
	FGameplayTag CombatSetTag; // e.g., Toolbar.Combat

	// ------------- Control -------------

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void ActivateSet(FGameplayTag SetTag);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void DeactivateSet(FGameplayTag SetTag);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void SetActiveSets(FGameplayTagContainer NewActiveSets);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void SetActiveIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void CycleNext();

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void CyclePrev();

	/** Back-compat wrappers expected by helpers */
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void TryCycleNext() { CycleNext(); }
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void TryCyclePrev() { CyclePrev(); }

	/** Zone & combat notifiers (used by SmartZone/ZoneControl). Safe no-ops if unconfigured. */
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void TryNotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Control")
	void TrySetCombatState(bool bInCombat);

	/** Try to wield the currently active slot */
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Actions")
	void WieldActiveSlot();

	// ------------- Query -------------

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Toolbar|Query")
	void GetActiveSetTags(FGameplayTagContainer& OutActive) const { OutActive = ActiveSetTags; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Toolbar|Query")
	void GetAllSetNames(TMap<FGameplayTag, FText>& OutNames) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Toolbar|Query")
	void GetActiveSetNames(TMap<FGameplayTag, FText>& OutNames) const;

	/** Union of active sets (mirrors ToolbarItemIDs) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Toolbar|Query")
	void GetComposedSlots(TArray<FGameplayTag>& OutSlots) const { OutSlots = ToolbarItemIDs; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Toolbar|Query")
	bool GetActiveSlotTag(FGameplayTag& OutSlot) const;

	// ------------- Events -------------

	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnToolbarChanged OnToolbarChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnActiveToolChanged OnActiveToolChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnToolbarVisibilityChanged OnToolbarVisibilityChanged;

protected:
	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Visible();

	UFUNCTION()
	void OnRep_ActiveIndex();

	UFUNCTION()
	void OnRep_ComposedSlots();

	// Server recomposition
	void RecomposeToolbar();

	// Helpers
	bool IsAuth() const;
	UEquipmentComponent* ResolveEquipment() const;
	UWieldComponent* ResolveWield() const;

	// RPCs
	UFUNCTION(Server, Reliable) void ServerActivateSet(FGameplayTag SetTag);
	UFUNCTION(Server, Reliable) void ServerDeactivateSet(FGameplayTag SetTag);
	UFUNCTION(Server, Reliable) void ServerSetActiveSets(FGameplayTagContainer NewActiveSets);
	UFUNCTION(Server, Reliable) void ServerSetActiveIndex(int32 NewIndex);
	UFUNCTION(Server, Reliable) void ServerWieldActiveSlot();

private:
	/** Replicated: which sets are active */
	UPROPERTY(ReplicatedUsing=OnRep_ComposedSlots)
	FGameplayTagContainer ActiveSetTags;

	/** Internal: canonical composed list (we mirror to ToolbarItemIDs for BP/back-compat) */
	UPROPERTY()
	TArray<FGameplayTag> ComposedSlots;
};
