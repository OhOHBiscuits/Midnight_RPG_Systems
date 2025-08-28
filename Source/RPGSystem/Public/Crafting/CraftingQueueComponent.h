#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingStationComponent.h"
#include "CraftingQueueComponent.generated.h"

class UCraftingRecipeDataAsset;
class UCraftingStationComponent;

UENUM(BlueprintType)
enum class ECraftQueueState : uint8
{
	Pending,
	InProgress,
	Done,
	Failed,
	Canceled
};

USTRUCT(BlueprintType)
struct FCraftQueueEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	FGuid EntryId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	TWeakObjectPtr<AActor> Submitter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	FCraftingRequest Request;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	ECraftQueueState State = ECraftQueueState::Pending;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	float TimeSubmitted = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	float TimeStarted = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQueueChanged, const TArray<FCraftQueueEntry>&, Queue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQueueEntryStarted, const FCraftQueueEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQueueEntryCompleted, const FCraftQueueEntry&, Entry, bool, bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingQueueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingQueueComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	TObjectPtr<UCraftingStationComponent> Station = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue", meta=(ClampMin="1", ClampMax="4"))
	int32 MaxConcurrent = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	bool bAutoStart = true;

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue", meta=(DefaultToSelf="Submitter"))
	FGuid EnqueueFor(AActor* Submitter, const FCraftingRequest& Request);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue", meta=(DefaultToSelf="Submitter"))
	int32 EnqueueBatchFor(AActor* Submitter, const TArray<FCraftingRequest>& Requests);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	FGuid Enqueue(const FCraftingRequest& Request);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	int32 EnqueueBatch(const TArray<FCraftingRequest>& Requests);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue", meta=(DefaultToSelf="Submitter"))
	int32 EnqueueRecipeFor(AActor* Submitter, UCraftingRecipeDataAsset* Recipe, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue", meta=(DefaultToSelf="Submitter"))
	int32 EnqueueRecipeBatchFor(AActor* Submitter, const TArray<UCraftingRecipeDataAsset*>& Recipes);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	bool RemovePending(const FGuid& EntryId);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	void ClearPending();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Crafting-Queue")
	const TArray<FCraftQueueEntry>& GetQueue() const { return Queue; }

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	void StartProcessing();

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue")
	void StopProcessing();

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue")
	FOnQueueChanged OnQueueChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue")
	FOnQueueEntryStarted OnEntryStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue")
	FOnQueueEntryCompleted OnEntryCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	TArray<FCraftQueueEntry> Queue;

	UPROPERTY(Replicated)
	bool bProcessing = false;

	UPROPERTY(Replicated)
	FGuid ActiveEntryId;

	UFUNCTION()
	void HandleStationStarted(const FCraftingRequest& Request, float FinalTime, const FSkillCheckResult& Check);

	UFUNCTION()
	void HandleStationCompleted(const FCraftingRequest& Request, const FSkillCheckResult& Check, bool bSuccess);

	void ResolveStation();
	void KickIfPossible();
	int32 FindIndexById(const FGuid& Id) const;
	int32 FindNextPending() const;

	static FCraftingRequest BuildRequestFromRecipe(AActor* Submitter, UCraftingRecipeDataAsset* Recipe);
};
