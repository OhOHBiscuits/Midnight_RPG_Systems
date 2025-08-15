#include "Actors/FuelWorkstationActor.h"
#include "Inventory/InventoryComponent.h"
#include "FuelSystem/FuelComponent.h"

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
		// Directly assign because UFuelComponent doesn't provide setters in your build
		FuelComponent->FuelInventory      = FuelInputInventory;
		FuelComponent->ByproductInventory = OutputInventory;
	}
}

void AFuelWorkstationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupFuelLinks();
}

void AFuelWorkstationActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	SetupFuelLinks();
}

void AFuelWorkstationActor::BeginPlay()
{
	Super::BeginPlay();
	SetupFuelLinks();
}

void AFuelWorkstationActor::StartBurn()
{
	if (FuelComponent) FuelComponent->StartBurn();
}

void AFuelWorkstationActor::StopBurn()
{
	if (FuelComponent) FuelComponent->StopBurn();
}

// If base declares UFUNCTION(BlueprintCallable) virtual void OpenWorkstationUIFor(AActor*);
void AFuelWorkstationActor::OpenWorkstationUIFor(AActor* Interactor)
{
	Super::OpenWorkstationUIFor(Interactor);
	// Optional: add station-specific pre/post UI logic here.
}

/*
// If base declares UFUNCTION(BlueprintNativeEvent) void OpenWorkstationUIFor(AActor*);
// Use this version instead (and comment the one above):
void AFuelWorkstationActor::OpenWorkstationUIFor_Implementation(AActor* Interactor)
{
	Super::OpenWorkstationUIFor_Implementation(Interactor);
}
*/

void AFuelWorkstationActor::OnCraftingActivated()
{
	// Keep lightweight & decoupled; no dependency on a specific UFuelComponent API.
	// You can trigger station "active" state here if you add such a flag later.
}
