#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces\InteractionInterface.h"
#include "Inventory\ItemDataAsset.h"
#include "PickupItemActor.generated.h"

UCLASS()
class RPGSYSTEM_API APickupItemActor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	APickupItemActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	UItemDataAsset* ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	int32 Quantity = 1;

	// Interact implementation
	virtual void Interact_Implementation(AActor* Interactor) override;
};
