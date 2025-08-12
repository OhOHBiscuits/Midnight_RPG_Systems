#include "Actors/StorageActor.h"
#include "Inventory/InventoryComponent.h"

AStorageActor::AStorageActor()
{
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	bUseEfficiency = false; // storage typically not using efficiency
}

void AStorageActor::BeginPlay()
{
	Super::BeginPlay();
	if (InventoryComp)
	{
		// Pull defaults from area at start
		InventoryComp->AutoInitializeAccessFromArea(this);
		// Set owner id if empty (when placed by a player this can be set at build time too)
		if (InventoryComp->Access.OwnerStableId.IsEmpty())
		{
			InventoryComp->Access.OwnerStableId = UInventoryHelpers::GetStablePlayerId(this);
		}
	}
}

void AStorageActor::Interact_Implementation(Actor* Interactor)
{
	Super::Interact_Implementation(Interactor);
	// Example: trigger UI based on view permission
	if (InventoryComp && InventoryComp->CanView(Interactor))
	{
		TriggerWorldItemUI(Interactor);
	}
}
