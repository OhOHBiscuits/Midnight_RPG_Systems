#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "PickupItemActor.generated.h"

UCLASS()
class RPGSYSTEM_API APickupItemActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	APickupItemActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	int32 Quantity = 1;

	virtual void Interact_Implementation(AActor* Interactor) override;
};
