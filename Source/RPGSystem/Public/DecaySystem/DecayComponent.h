#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DecayComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;

USTRUCT(BlueprintType)
struct FDecaySlot
{
	GENERATED_BODY()

	UPROPERTY() int32 SlotIndex = -1;
	UPROPERTY() float DecayTimeRemaining = -1.f;
	UPROPERTY() float TotalDecayTime = -1.f;
	UPROPERTY() int32 BatchSize = 1;

	FDecaySlot() {}
	FDecaySlot(int32 InSlot, float InTime, float InTotal, int32 InBatch)
		: SlotIndex(InSlot), DecayTimeRemaining(InTime), TotalDecayTime(InTotal), BatchSize(InBatch) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDecayProgressEvent, int32, SlotIndex, float, RemainingSeconds, float, TotalSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemDecayed, int32, SlotIndex, UItemDataAsset*, NewItemData, int32, OutputQuantity);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGSYSTEM_API UDecayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDecayComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay|Debug")
	bool bLogDecayDebug = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	AActor* SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	UInventoryComponent* Inventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	float DecayCheckInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	float DecaySpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	float EfficiencyRating = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	int32 InputBatchSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay")
	bool bDecayActive = true;

	UPROPERTY(BlueprintAssignable, Category="Decay")
	FDecayProgressEvent OnDecayProgress;

	UPROPERTY(BlueprintAssignable, Category="Decay")
	FOnItemDecayed OnItemDecayed;

	UFUNCTION()
	void OnInventoryChangedRefresh();


protected:
	FTimerHandle DecayTimerHandle;

	UPROPERTY() TArray<FDecaySlot> DecaySlots;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION() void DecayTimerTick();

	void ResolveInventory();
	void ApplySettingsFromInventoryType();
	void RefreshDecaySlots();

	virtual bool ShouldDecayItem(const struct FInventoryItem& Item, UItemDataAsset* Asset) const { return true; }
	int32 CalculateBatchOutput(int32 InputUsed) const;

	// Reads seconds from ItemDataAsset (uses DecayRate seconds in your current asset)
	float GetItemDecaySeconds(const UItemDataAsset* Asset) const;

	// Helpers that soft-load assets when needed (Standalone/cooked-safe)
	static UItemDataAsset* ResolveItemAsset(const struct FInventoryItem& SlotItem);
	static UItemDataAsset* ResolveSoftItem(UItemDataAsset* MaybeLoaded, const TSoftObjectPtr<UItemDataAsset>& Soft);
};
