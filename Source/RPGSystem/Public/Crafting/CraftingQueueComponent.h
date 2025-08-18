#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingQueueComponent.generated.h"

class UCraftingRecipeDataAsset;
class UAbilitySystemComponent;
struct FGameplayEventData; // fwd declare OK for pointers

USTRUCT(BlueprintType)
struct FCraftQueueEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	FGameplayTag RecipeIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue", meta=(ClampMin="1"))
	int32 Quantity = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	FGuid JobId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue", meta=(Categories="Craft.Job"))
	FGameplayTag StateTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue")
	float ETASeconds = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCraftQueueChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftQueueJobEvent, const FGuid&, JobId, const FGameplayTag&, RecipeID);

/** Server-authoritative craft queue */
UCLASS(ClassGroup=(Crafting), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingQueueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingQueueComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue|Config")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue|Config", meta=(ClampMin="1", ClampMax="4"))
	int32 MaxConcurrent = 1;

	UPROPERTY(ReplicatedUsing=OnRep_Queue, VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Queue|State")
	TArray<FCraftQueueEntry> Queue;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue|Events")
	FOnCraftQueueChanged OnQueueChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue|Events")
	FOnCraftQueueJobEvent OnJobStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Queue|Events")
	FOnCraftQueueJobEvent OnJobFinished;

	// Server-only
	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue|Actions")
	bool EnqueueRecipe(UCraftingRecipeDataAsset* Recipe, int32 Quantity, AActor* Workstation);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue|Actions")
	bool EnqueueByTag(FGameplayTag RecipeIDTag, int32 Quantity, AActor* Workstation);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue|Actions")
	bool CancelAtIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue|Actions")
	void ClearQueued();

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Queue|Actions")
	void StartProcessing();

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void OnRep_Queue();

private:
	struct FServerJob { TWeakObjectPtr<UCraftingRecipeDataAsset> Recipe; TWeakObjectPtr<AActor> Workstation; };
	TMap<FGuid, FServerJob> ServerJobs; // server-only
	TSet<FGuid> InFlight;               // server-only
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	void TryStartNext();
	bool CanStartMore() const;
	void StartJob(FCraftQueueEntry& Entry, const FServerJob& ServerJob);

	void BindASCEvents();
	void OnCraftStartedEvent(const FGameplayEventData* EventData);
	void OnCraftCompletedEvent(const FGameplayEventData* EventData);
};
