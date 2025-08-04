#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "Inventory/InventoryComponent.h"
#include "StorageActor.generated.h"

UCLASS()
class RPGSYSTEM_API AStorageActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	AStorageActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Storage")
	UInventoryComponent* InventoryComp;

	virtual void Interact_Implementation(AActor* Interactor) override;
};
