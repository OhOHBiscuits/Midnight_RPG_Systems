#pragma once

#include "CoreMinimal.h"
#include "Actors/WorkstationActor.h"
#include "FuelWorkstationActor.generated.h"

class UInventoryComponent;
class UFuelComponent;

/**
 * A workstation that consumes fuel:
 * - Has a fuel input inventory
 * - Has an output/byproduct inventory
 * - Uses a FuelComponent to burn & generate byproducts
 */
UCLASS()
class RPGSYSTEM_API AFuelWorkstationActor : public AWorkstationActor
{
	GENERATED_BODY()

public:
	AFuelWorkstationActor();

protected:
	/** Inventory where fuel items are inserted. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* FuelInputInventory;

	/** Byproducts (e.g., ash/charcoal) collected here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* OutputInventory;

	/** Responsible for consuming fuel & ticking burn. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UFuelComponent* FuelComponent;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Fuel")
	void SetupFuelLinks();
	

	// AWorkstationActor
	virtual void OpenWorkstationUIFor(AActor* Interactor) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	// You can also override HandleInteract_Server if you want custom behavior before/after UI
	// virtual void HandleInteract_Server(AActor* Interactor) override;
};
