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

	// NOTE: Base class already declares this as a UFUNCTION. Do NOT redeclare UFUNCTION here.
	virtual void OpenWorkstationUIFor(AActor* Interactor) override;
	// If your base is BlueprintNativeEvent instead, switch to:
	// virtual void OpenWorkstationUIFor_Implementation(AActor* Interactor) override;

	// Optional hook you can call from UI or logic
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel")
	void OnCraftingActivated();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

private:
	void SetupFuelLinks();
};
