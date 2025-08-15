#include "Actors/FuelWorkstationActor.h"
#include "Inventory/InventoryComponent.h"
#include "FuelSystem/FuelComponent.h"
#include "Blueprint/UserWidget.h"

AFuelWorkstationActor::AFuelWorkstationActor()
{
	// Create components (both are UActorComponent, not scene components)
	FuelInputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("FuelInputInventory"));
	OutputInventory    = CreateDefaultSubobject<UInventoryComponent>(TEXT("OutputInventory"));
	FuelComponent      = CreateDefaultSubobject<UFuelComponent>(TEXT("FuelComponent"));

	// Replication (optional but usually desired)
	if (FuelInputInventory) FuelInputInventory->SetIsReplicated(true);
	if (OutputInventory)    OutputInventory->SetIsReplicated(true);
	if (FuelComponent)      FuelComponent->SetIsReplicated(true);

	// Workstations usually care about efficiency
	bUseEfficiency = true;
}

void AFuelWorkstationActor::OpenWorkstationUIFor(AActor* Interactor)
{
	// Optional: gate UI if there’s no fuel burning, or if some safety is needed.
	// (Uncomment if desired)
	// if (FuelComponent && !FuelComponent->IsBurning())
	// {
	//     // Could show a “no fuel” notification instead of opening the full UI.
	//     return;
	// }

	// Otherwise, call the parent to use the shared UI pipeline.
	Super::OpenWorkstationUIFor(Interactor);
}

void AFuelWorkstationActor::SetupFuelLinks()
{
	if (FuelComponent)
	{
		FuelComponent->SetFuelInventory(FuelInputInventory);
		FuelComponent->SetByproductInventory(OutputInventory);
	}
}

void AFuelWorkstationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupFuelLinks();            // editor-time (also runs in PIE)
}

void AFuelWorkstationActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	SetupFuelLinks();            // runtime (both server & clients)
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();
	SetupFuelLinks();            // extra safety
}