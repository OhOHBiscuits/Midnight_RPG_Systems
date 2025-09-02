// Source/RPGSystem/Public/Actors/FuelWorkstationActor.h
#pragma once

#include "CoreMinimal.h"
#include "Actors/WorkstationActor.h"
#include "FuelWorkstationActor.generated.h"

class UFuelComponent;
class UInventoryComponent;

/**
 * Workstation that consumes fuel (campfire / forge / cooker, etc.)
 * Inherits the OutputInventory and CraftingStation from AWorkstationActor.
 */
UCLASS()
class RPGSYSTEM_API AFuelWorkstationActor : public AWorkstationActor
{
	GENERATED_BODY()

public:
	AFuelWorkstationActor();

	// Fuel slot inventory (input)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	TObjectPtr<UInventoryComponent> FuelInputInventory;

	// Fuel logic
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	TObjectPtr<UFuelComponent> FuelComponent;

protected:
	virtual void BeginPlay() override;

	// Optional: hook for UI; base class already routes to TriggerWorldItemUI
	
};
