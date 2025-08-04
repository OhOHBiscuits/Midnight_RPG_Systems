#include "Actors/FuelWorkstationActor.h"
#include "Inventory/InventoryComponent.h"
#include "FuelSystem/FuelComponent.h"

AFuelWorkstationActor::AFuelWorkstationActor()
{
	// Don't tick - we use timers only
	PrimaryActorTick.bCanEverTick = false;

	FuelInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("FuelInventory"));
	ByproductInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("ByproductInventory"));
	FuelComponent = CreateDefaultSubobject<UFuelComponent>(TEXT("FuelComponent"));

	// Optionally: Set as root component if you want
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	// Ensure component pointers are valid
	if (FuelComponent)
	{
		FuelComponent->FuelInventory = FuelInventory;
		FuelComponent->ByproductInventory = ByproductInventory;
	}
}

void AFuelWorkstationActor::OnCraftingActivated()
{
	// Optional hook - can be expanded for specific stations, crafting logic, etc
}
