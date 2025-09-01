#pragma once

#include "CoreMinimal.h"
#include "Actors/WorkstationActor.h"
#include "FuelWorkstationActor.generated.h"

class UInventoryComponent;

UCLASS()
class RPGSYSTEM_API AFuelWorkstationActor : public AWorkstationActor
{
	GENERATED_BODY()
public:
	AFuelWorkstationActor();

protected:
	virtual void BeginPlay() override;

	// Where crafted items should land for this station variant
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UInventoryComponent> OutputInventory = nullptr;
};
