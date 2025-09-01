#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingStationComponent.generated.h"

class UCraftingRecipeDataAsset;
class UInventoryComponent;

/** Minimal output/cost structs – use yours if you already have them */
USTRUCT(BlueprintType)
struct FCraftItemCost { GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag ItemID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32       Quantity = 1;
};
USTRUCT(BlueprintType)
struct FCraftItemOutput { GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<class UItemDataAsset> Item = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 Quantity = 1;
};

/** A queued job (kept simple; server authoritative) */
USTRUCT(BlueprintType)
struct FCraftingJob
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FGameplayTag RecipeID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Count = 1;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TotalTime = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TimeRemaining = 0.f;

	// Not replicated: server uses this to process/award XP
	UPROPERTY(Transient) TObjectPtr<const UCraftingRecipeDataAsset> Recipe = nullptr;
	UPROPERTY(Transient) TWeakObjectPtr<AActor> Instigator;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftStartedSimple, const FCraftingJob&, Job);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftFinishedSimple, const FCraftingJob&, Job, bool, bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCraftingStationComponent();

	/** Classification used to filter recipes (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	FGameplayTag StationDiscipline;                       // e.g. Crafting.Cooking

	/** Tags this station provides (recipes may require some of them) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	FGameplayTagContainer StationTags;                    // e.g. Workstation.Campfire, Heat.Source

	/** Option A: explicitly assign recipes here */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>> StationRecipes;

	/** Option B: allow any recipe with these tags (by RecipeIDTag or a custom tag you choose) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	FGameplayTagContainer AllowedRecipeTags;

	/** If craft time <= this, we do it instantly (no queue wait) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup", meta=(ClampMin="0.0"))
	float InstantThresholdSeconds = 0.25f;

	/** If true we’ll queue extra crafts when one is running */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	bool bEnableQueue = true;

	/** Events */
	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Events") FOnCraftStartedSimple  OnCraftStarted;
	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Events") FOnCraftFinishedSimple OnCraftFinished;

	/** Query helpers for UI */
	UFUNCTION(BlueprintCallable, Category="1_Crafting-Query")
	void GetAllStationRecipes(TArray<UCraftingRecipeDataAsset*>& Out) const;

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Query")
	void GetCraftableRecipes(AActor* Viewer, TArray<UCraftingRecipeDataAsset*>& OutCraftable, TArray<UCraftingRecipeDataAsset*>& OutLocked) const;

	/** Entry points */
	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions", meta=(DefaultToSelf="Instigator"))
	bool StartCraftFromRecipe(AActor* Instigator, UCraftingRecipeDataAsset* Recipe, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions", meta=(DefaultToSelf="Instigator"))
	bool StartCraftByTag(AActor* Instigator, FGameplayTag RecipeID, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions")
	void CancelActive();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Queue & state
	UPROPERTY(VisibleAnywhere) TArray<FCraftingJob> Queue;
	UPROPERTY(VisibleAnywhere) bool bProcessing = false;
	FTimerHandle CraftTimer;

	// Core flow
	bool ResolveRecipeByTag(FGameplayTag RecipeID, const UCraftingRecipeDataAsset*& OutRecipe) const;
	bool PassesStationFilters(const UCraftingRecipeDataAsset* Recipe) const;
	bool IsRecipeUnlocked(AActor* Viewer, const UCraftingRecipeDataAsset* Recipe) const;
	bool HasAndConsumeInputs(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe, int32 Count); // TODO: implement with your inventory helpers
	void GrantOutputsAndXP(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe, int32 Count, bool bSuccess);

	void Enqueue(const UCraftingRecipeDataAsset* Recipe, AActor* Instigator, int32 Count);
	void TryStartNext();
	void TickActive();
	void FinishActive(bool bSuccess);
};
