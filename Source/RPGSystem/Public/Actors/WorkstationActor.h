#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "Inventory/InventoryComponent.h"
#include "WorkstationActor.generated.h"

UCLASS()
class RPGSYSTEM_API AWorkstationActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	AWorkstationActor();
	

	virtual void Interact_Implementation(AActor* Interactor) override;
};
