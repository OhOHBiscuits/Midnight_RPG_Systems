#pragma once

#include "CoreMinimal.h"
#include "Actors/WorkstationActor.h"
#include "Inventory/InventoryComponent.h"
#include "FuelSystem/FuelComponent.h"
#include "FuelWorkstationActor.generated.h"

UCLASS()
class RPGSYSTEM_API AFuelWorkstationActor : public AWorkstationActor
{
	GENERATED_BODY()

public:
	AFuelWorkstationActor();

	// Input/output inventories, exposed for editing & UI
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* FuelInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UInventoryComponent* ByproductInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	UFuelComponent* FuelComponent;

	// --- Blueprint convenience: accessors ---
	UFUNCTION(BlueprintCallable, Category="Fuel")
	FORCEINLINE UFuelComponent* GetFuelComponent() const { return FuelComponent; }

	UFUNCTION(BlueprintCallable, Category="Fuel")
	FORCEINLINE UInventoryComponent* GetFuelInventory() const { return FuelInventory; }

	UFUNCTION(BlueprintCallable, Category="Fuel")
	FORCEINLINE UInventoryComponent* GetByproductInventory() const { return ByproductInventory; }

	// Helper: quick start/stop burning
	UFUNCTION(BlueprintCallable, Category="Fuel")
	void StartBurn() { if (FuelComponent) FuelComponent->StartBurn(); }

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void StopBurn() { if (FuelComponent) FuelComponent->StopBurn(); }

	// --- Expansion: Add crafting/other hooks here ---
	UFUNCTION(BlueprintCallable, Category="Fuel")
	void OnCraftingActivated();

protected:
	virtual void BeginPlay() override;
};
