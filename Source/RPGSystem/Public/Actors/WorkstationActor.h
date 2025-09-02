#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "WorkstationActor.generated.h"

class UCraftingStationComponent;
class UInventoryComponent;
class UCraftingRecipeDataAsset;
class UWorkstationDataAsset;

UCLASS()
class RPGSYSTEM_API AWorkstationActor : public ABaseWorldItemActor
{
	GENERATED_BODY()

public:
	AWorkstationActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Interact_Implementation(AActor* Interactor) override;

	/** Core crafting logic lives here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crafting")
	TObjectPtr<UCraftingStationComponent> CraftingStation;

	/** Where finished items are delivered by default. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crafting")
	TObjectPtr<UInventoryComponent>      OutputInventory;

	/** Data-driven configuration (recipes, UI, tags…). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation")
	TObjectPtr<UWorkstationDataAsset>    StationConfig;

	/** Returns the hard-loaded recipes (resolved from StationConfig). */
	UFUNCTION(BlueprintCallable, Category="Workstation")
	const TArray<UCraftingRecipeDataAsset*>& GetAvailableRecipes();

	/** Is this recipe allowed by the station data? (loads soft reference if needed) */
	UFUNCTION(BlueprintCallable, Category="Workstation")
	bool IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe);

private:
	// runtime cache only (not authored) so UI/crafting can use hard pointers
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCraftingRecipeDataAsset>> CachedRecipes;

	void ResolveRecipesSync(); // simple, safe sync load; swap to async later if desired
};
