#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DecayComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;
struct FInventoryItem;

USTRUCT(BlueprintType)
struct FDecaySlot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SlotIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DecayTimeRemaining = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TotalDecayTime = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BatchSize = 1;

	FDecaySlot() {}
	FDecaySlot(int32 InSlot, float InRemain, float InTotal, int32 InBatch)
		: SlotIndex(InSlot), DecayTimeRemaining(InRemain), TotalDecayTime(InTotal), BatchSize(InBatch) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDecayProgressEvent, int32, SlotIndex, float, RemainingSeconds, float, TotalSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemDecayed, int32, SlotIndex, UItemDataAsset*, OutputItem, int32, OutputQty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackingStateChanged, bool, bIsTracking);

/**
 * Add-on component that drives item decay inside an InventoryComponent.
 * Server-authoritative. Replicates progress for UI.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGSYSTEM_API UDecayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDecayComponent();

	// --- Setup ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Decay|Setup")
	AActor* SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Decay|Setup")
	UInventoryComponent* Inventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Decay|Setup")
	bool bAutoResolveInventory = true;

	// --- Control (Server) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory-Decay|Control")
	bool bDecayActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory-Decay|Control")
	float DecayCheckInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Decay|Control")
	bool bAutoStopWhenIdle = true;

	// --- Tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory-Decay|Tuning")
	float DecaySpeedMultiplier = 1.0f;   // higher = faster

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory-Decay|Tuning")
	float EfficiencyRating = 1.0f;       // higher = more output per batch

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory-Decay|Tuning")
	int32 InputBatchSize = 1;            // items consumed per completion

	// --- State (Rep) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_DecaySlots, Category="1_Inventory-Decay|State")
	TArray<FDecaySlot> DecaySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_IsTrackingAny, Category="1_Inventory-Decay|State")
	bool bIsTrackingAny = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Decay|Debug")
	bool bLogDecayDebug = false;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Decay|Events")
	FDecayProgressEvent OnDecayProgress;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Decay|Events")
	FOnItemDecayed OnItemDecayed;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Decay|Events")
	FOnTrackingStateChanged OnTrackingStateChanged;

	// --- API ---
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void StartDecay();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void StopDecay();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void ForceRefresh();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void RebindToInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void SetInventoryByActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory-Decay|Queries")
	bool GetSlotProgress(int32 SlotIndex, float& OutRemaining, float& OutTotal) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory-Decay|Queries")
	bool IsIdle() const;

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Decay|Control")
	void StopIfIdle();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	FTimerHandle DecayTimerHandle;

	bool bTickInProgress = false;
	bool bRefreshRequested = false;

	// Core
	void ResolveInventory();
	void BindInventoryChanged(bool bBind);
	void ApplySettingsFromInventoryType();
	int32 RefreshDecaySlots();
	void TryStartStopFromCurrentState();
	void StartTimer();
	void StopTimer();
	bool HasAuthoritySafe() const;
	bool IsInventoryValid() const { return Inventory != nullptr && GetOwner() && GetOwner()->HasAuthority(); }


	// Tick
	UFUNCTION()
	void DecayTimerTick();

	// Helpers
	static UItemDataAsset* ResolveItemAsset(const FInventoryItem& SlotItem);
	static UItemDataAsset* ResolveSoftItem(UItemDataAsset* MaybeLoaded, const TSoftObjectPtr<UItemDataAsset>& Soft);
	float GetItemDecaySeconds(const UItemDataAsset* Asset) const;
	int32 CalculateBatchOutput(int32 InputUsed) const;
	bool ConsumeInputAtSlot_Server(int32 SlotIndex, int32 Quantity);
	void CompactTrackedSlots();

	// Inventory change hook
	UFUNCTION()
	void OnInventoryChangedRefresh();

	// RepNotifies
	UFUNCTION()
	void OnRep_DecaySlots();

	UFUNCTION()
	void OnRep_IsTrackingAny();

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void Server_StartDecay();
	UFUNCTION(Server, Reliable, WithValidation) void Server_StopDecay();
};
