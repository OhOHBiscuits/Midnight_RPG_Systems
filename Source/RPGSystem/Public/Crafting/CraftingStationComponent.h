// CraftingStationComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Crafting/CraftingTypes.h"
#include "CraftingStationComponent.generated.h"

class UCraftingRecipeDataAsset;
class UInventoryComponent;
class AWorkstationActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingStartedSignature, const FCraftingJob&, Job);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCraftingFinishedSignature, const FCraftingJob&, Job, bool, bSuccess);

UCLASS(ClassGroup=(RPGSystem), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingStationComponent();

	// ---------------------------------------------------------------------
	// Inventories (wired by the owning actor)
	// ---------------------------------------------------------------------
	/** Materials are staged here while the job runs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting|Inventories")
	TObjectPtr<UInventoryComponent> InputInventory = nullptr;

	/** Results are deposited here on success. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting|Inventories")
	TObjectPtr<UInventoryComponent> OutputInventory = nullptr;

	// ---------------------------------------------------------------------
	// Replicated state
	// ---------------------------------------------------------------------
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Crafting|State")
	FCraftingJob ActiveJob;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Crafting|State")
	bool bIsCrafting = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Crafting|State")
	bool bIsPaused = false;

	// ---------------------------------------------------------------------
	// API
	// ---------------------------------------------------------------------
	/** Try to start crafting a recipe (Times = how many to craft back-to-back). */
	UFUNCTION(BlueprintCallable, Category="Crafting")
	bool StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe, int32 Times = 1);

	/** Cancel the running job (materials can be returned from Input if you want). */
	UFUNCTION(BlueprintCallable, Category="Crafting")
	void CancelCraft();

	/** Pause current job (timer is stopped; materials remain in Input). */
	UFUNCTION(BlueprintCallable, Category="Crafting")
	void PauseCraft();

	/** Resume a paused job. */
	UFUNCTION(BlueprintCallable, Category="Crafting")
	void ResumeCraft();

	/** Convenience: collect all gameplay tags owned by an actor (self/PS/PC/ASC). */
	UFUNCTION(BlueprintCallable, Category="Crafting|Utility")
	void GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const;

	// Events
	UPROPERTY(BlueprintAssignable, Category="Crafting|Events")
	FCraftingStartedSignature OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="Crafting|Events")
	FCraftingFinishedSignature OnCraftFinished;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called by timers; finishes current craft tick. */
	void FinishCraft_Internal();

	// --- Hook points to your inventory system (kept minimal so this compiles clean) ---
	/** Move the required materials (Times) into the Input inventory. Return false if you can’t stage everything. */
	bool MoveRequiredToInput(const UCraftingRecipeDataAsset* Recipe, int32 Times);

	/** Give outputs from Recipe to Output inventory. */
	void DeliverOutputs(const UCraftingRecipeDataAsset* Recipe);

	/** Optional XP hook. */
	void GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* Recipe, AActor* InstigatorActor, bool bSuccess);

private:
	FTimerHandle CraftTimerHandle;
};
