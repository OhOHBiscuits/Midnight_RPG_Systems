#include "Actors/PickupItemActor.h"
#include "Inventory\InventoryHelpers.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

APickupItemActor::APickupItemActor()
{
	bUseEfficiency = false; // Pickups do NOT use efficiency
}
void APickupItemActor::Interact_Implementation(AActor* Interactor)
{
	Super::Interact_Implementation(Interactor);
	if (!HasAuthority())
		return;

	if (!ItemData || Quantity <= 0 || !Interactor) return;

	UInventoryComponent* Inv = UInventoryHelpers::GetInventoryComponent(Interactor);
	if (Inv && Inv->TryAddItem(ItemData, Quantity))
	{
		Destroy();
	}
	else
	{
		TriggerWorldItemUI(Interactor);
	}
}

	
