// Source/RPGSystem/Private/Actors/FuelWorkstationActor.cpp
#include "Actors/FuelWorkstationActor.h"
#include "FuelSystem/FuelComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Crafting/CraftingStationComponent.h"

AFuelWorkstationActor::AFuelWorkstationActor()
{
	// Components are UActorComponent (non-scene) so NO SetupAttachment here
	FuelInputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("FuelInputInventory"));
	FuelComponent      = CreateDefaultSubobject<UFuelComponent>(TEXT("FuelComponent"));

	// OutputInventory and CraftingStation are created in AWorkstationActor
	// Nothing to set on CraftingStation here (it doesn't own an OutputInventory member)
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	// If your UFuelComponent exposes a property for the inventory, wire it here.
	// (Earlier errors showed no SetFuelInventory(), so skip unless you add one.)
	// Example if you later add it:
	// if (FuelComponent) { FuelComponent->SetFuelInventory(FuelInputInventory); }
}


