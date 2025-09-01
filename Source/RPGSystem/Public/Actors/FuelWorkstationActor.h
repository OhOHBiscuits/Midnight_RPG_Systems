#pragma once

#include "CoreMinimal.h"
#include "Actors/WorkstationActor.h"
#include "FuelWorkstationActor.generated.h"

class UInventoryComponent;
class UFuelComponent;

UCLASS()
class RPGSYSTEM_API AFuelWorkstationActor : public AWorkstationActor
{
	GENERATED_BODY()

public:
	AFuelWorkstationActor();

	// Components (replicated)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* FuelInputInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* OutputInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UFuelComponent* FuelComponent;

	// Simple server-auth controls
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel")
	void StartBurn();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel")
	void StopBurn();

	// Optional hook
	UFUNCTION(BlueprintCallable, Category="1_Crafting-Logic")
	void OnCraftingActivated();

	// UI passthrough
	virtual void OpenWorkstationUIFor(AActor* Interactor) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;	
private:
	void SetupFuelLinks();
	void SetupCraftingOutputRouting();
};
