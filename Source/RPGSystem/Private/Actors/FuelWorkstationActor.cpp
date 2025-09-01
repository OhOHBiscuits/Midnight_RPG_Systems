#include "Actors/FuelWorkstationActor.h"
#include "Crafting/CraftingStationComponent.h"
#include "Inventory/InventoryComponent.h"

AFuelWorkstationActor::AFuelWorkstationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// NOTE: UCraftingStationComponent is created in AWorkstationActor (CraftingStation).
	// Do NOT call SetupAttachment on it (it’s not a USceneComponent).

	OutputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("OutputInventory"));
	// Also a UActorComponent, so no SetupAttachment here either.
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	if (CraftingStation)
	{
		// These properties are defined on UCraftingStationComponent (below)
		CraftingStation->bOutputToStationInventory = true;
		CraftingStation->OutputInventoryOverride   = OutputInventory;
	}
}
