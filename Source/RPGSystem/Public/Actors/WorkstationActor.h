#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCraftingStationComponent> CraftingStation;

	// Expose a simple helper for UI to call
	UFUNCTION(BlueprintCallable, Category="Crafting")
	bool StartRecipe(UCraftingRecipeDataAsset* Recipe, int32 Quantity = 1);

	// ---------- NEW: base virtual so derived classes (e.g., AFuelWorkstationActor) can override ----------
	// Called to open the workstation UI for an interactor (player, etc).
	// Default impl: no-op. Override in derived classes to actually open a widget/HUD.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void OpenWorkstationUIFor(AActor* Interactor);

protected:
	virtual void BeginPlay() override;

	// Station event hooks
	UFUNCTION()
	void HandleCraftFinished(const FCraftingJob& Job, bool bSuccess);
};
