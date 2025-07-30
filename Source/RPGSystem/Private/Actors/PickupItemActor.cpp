#include "Actors\PickupItemActor.h"
#include "Inventory/InventoryComponent.h"

APickupItemActor::APickupItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void APickupItemActor::Interact_Implementation(AActor* Interactor)
{
	if (!ItemData || Quantity <= 0 || !Interactor) return;

	UInventoryComponent* Inv = Interactor->FindComponentByClass<UInventoryComponent>();
	if (Inv && Inv->TryAddItem(ItemData, Quantity))
	{
		Destroy();
	}
	// Optional: else play "inventory full" sound or feedback, or drop some but not all, etc.
}
