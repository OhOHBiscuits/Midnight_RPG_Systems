#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Crafting/CraftingTypes.h"
#include "WorkstationActor.generated.h"

class UCraftingStationComponent;
class UCraftingRecipeDataAsset;

UCLASS()
class RPGSYSTEM_API AWorkstationActor : public AActor
{
	GENERATED_BODY()

public:
	AWorkstationActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCraftingStationComponent> CraftingStation;

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleCraftStarted(const FCraftingJob& Job);

	UFUNCTION()
	void HandleCraftFinished(const FCraftingJob& Job, bool bSuccess);

public:
	UFUNCTION(BlueprintCallable, Category="Workstation")
	bool StartRecipe(const UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintCallable, Category="Workstation")
	void CancelActiveCraft();
};
