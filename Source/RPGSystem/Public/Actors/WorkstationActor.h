// WorkstationActor.h
#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "WorkstationActor.generated.h"

class UCraftingStationComponent;
class UInventoryComponent;
class UCraftingRecipeDataAsset;
class UWorkstationDataAsset;
class UUserWidget;

UCLASS()
class RPGSYSTEM_API AWorkstationActor : public ABaseWorldItemActor
{
	GENERATED_BODY()

public:
	AWorkstationActor();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation")
	TObjectPtr<UCraftingStationComponent> CraftingStation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Inventory")
	TObjectPtr<UInventoryComponent>      InputInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Inventory")
	TObjectPtr<UInventoryComponent>      OutputInventory;

	// Data-driven setup
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation")
	TObjectPtr<UWorkstationDataAsset>    WorkstationData = nullptr;

	// UI to show when interacted
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation|UI")
	TSubclassOf<UUserWidget>             WorkstationUIClass;

	/** Simple allow-list gate. */
	UFUNCTION(BlueprintCallable, Category="Workstation")
	bool IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe) const;

	// Interaction
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void OnInteract(AActor* Interactor);
	virtual void OnInteract_Implementation(AActor* Interactor); // not marked override on purpose

protected:
	virtual void BeginPlay() override;
};
