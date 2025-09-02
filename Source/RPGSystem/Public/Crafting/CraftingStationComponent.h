// CraftingStationComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.h" // defines FCraftingJob (do NOT duplicate it here)
#include "CraftingStationComponent.generated.h"

class UCraftingRecipeDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftStarted,  const FCraftingJob&, Job);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftFinished, const FCraftingJob&, Job, bool, bSuccess);

UCLASS(ClassGroup=(RPG), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingStationComponent();

	// Events for UI/triggers
	UPROPERTY(BlueprintAssignable, Category="Crafting|Events")
	FOnCraftStarted OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="Crafting|Events")
	FOnCraftFinished OnCraftFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	TObjectPtr<UInventoryComponent> OutputInventory = nullptr;	
	

	// Start/cancel
	UFUNCTION(BlueprintCallable, Category="Crafting")
    	bool StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting|Recipes")
	TArray<TObjectPtr<UCraftingRecipeDataAsset>> StationRecipes;

	UFUNCTION(BlueprintCallable, Category="Crafting")
	void CancelCraft();

	// Helpers
	UFUNCTION(BlueprintPure, Category="Crafting")
	bool IsCrafting() const { return bIsCrafting; }

	UFUNCTION(BlueprintCallable, Category="Crafting|Tags")
	void GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const;

	// Add/remove/query helpers 
	UFUNCTION(BlueprintCallable, Category="Crafting|Recipes")
	void AddRecipe(UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintCallable, Category="Crafting|Recipes")
	void RemoveRecipe(UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintPure, Category="Crafting|Recipes")
	bool HasRecipe(const UCraftingRecipeDataAsset* Recipe) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Internals
	void FinishCraft_Internal(bool bSuccess);
	void DeliverOutputs(const UCraftingRecipeDataAsset* Recipe, AActor* Instigator);
	void GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* Recipe, AActor* Instigator, bool bSuccess);

	UPROPERTY(Replicated)
	FCraftingJob ActiveJob;

	UPROPERTY(Replicated)
	bool bIsCrafting = false;

	FTimerHandle CraftTimerHandle;
};
