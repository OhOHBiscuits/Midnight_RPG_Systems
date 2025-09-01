#include "Actors/FuelWorkstationActor.h"
#include "Inventory/InventoryComponent.h"
#include "FuelSystem/FuelComponent.h"

#include "Crafting/CraftingStationComponent.h"

AFuelWorkstationActor::AFuelWorkstationActor()
{
	FuelInputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("FuelInputInventory"));
	OutputInventory    = CreateDefaultSubobject<UInventoryComponent>(TEXT("OutputInventory"));
	FuelComponent      = CreateDefaultSubobject<UFuelComponent>(TEXT("FuelComponent"));

	if (FuelInputInventory) FuelInputInventory->SetIsReplicated(true);
	if (OutputInventory)    OutputInventory->SetIsReplicated(true);
	if (FuelComponent)      FuelComponent->SetIsReplicated(true);
}

void AFuelWorkstationActor::SetupFuelLinks()
{
	if (FuelComponent)
	{
		FuelComponent->FuelInventory      = FuelInputInventory;
		FuelComponent->ByproductInventory = OutputInventory;
	}
}

void AFuelWorkstationActor::SetupCraftingOutputRouting()
{
	// Route crafted outputs to this actor's OutputInventory by default.
	if (CraftingStation && OutputInventory)
	{
		CraftingStation->bOutputToStationInventory = true;
		CraftingStation->OutputInventoryOverride   = OutputInventory;
	}
}

void AFuelWorkstationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupFuelLinks();
	SetupCraftingOutputRouting();
}

void AFuelWorkstationActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	SetupFuelLinks();
	SetupCraftingOutputRouting();
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();
	SetupFuelLinks();
	SetupCraftingOutputRouting();
}

void AFuelWorkstationActor::StartBurn()
{
	if (FuelComponent) FuelComponent->StartBurn();
}

void AFuelWorkstationActor::StopBurn()
{
	if (FuelComponent) FuelComponent->StopBurn();
}

void AFuelWorkstationActor::OpenWorkstationUIFor(AActor* Interactor)
{
	Super::OpenWorkstationUIFor(Interactor);
	// Station-specific pre/post UI logic can go here if needed.
}

void AFuelWorkstationActor::OnCraftingActivated()
{
	// Optional hook (e.g., tick fuel, toggle fire FX, etc.)
}
