// WorkstationActor.cpp
#include "Actors/WorkstationActor.h"

#include "Crafting/CraftingStationComponent.h"
#include "Inventory/WorkstationDataAsset.h"

// If your inventory component is in another folder, adjust the include:
#include "Inventory/InventoryComponent.h"

#include "Blueprint/UserWidget.h"

AWorkstationActor::AWorkstationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CraftingStation = CreateDefaultSubobject<UCraftingStationComponent>(TEXT("CraftingStation"));

	InputInventory  = CreateDefaultSubobject<UInventoryComponent>(TEXT("InputInventory"));
	OutputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("OutputInventory"));

	// If you have a root, attach here; otherwise the components will attach to root by default.
	// InputInventory->SetupAttachment(GetRootComponent());
	// OutputInventory->SetupAttachment(GetRootComponent());
	// CraftingStation->SetupAttachment(GetRootComponent());
}

void AWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	// Wire inventories into the station so it knows where to stage and where to output.
	if (CraftingStation)
	{
		CraftingStation->InputInventory  = InputInventory;
		CraftingStation->OutputInventory = OutputInventory;
	}
}

bool AWorkstationActor::IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe) const
{
	return (WorkstationData ? WorkstationData->IsRecipeAllowed(Recipe) : true);
}

void AWorkstationActor::OnInteract_Implementation(AActor* Interactor)
{
		Client_ShowWorldItemUI(Interactor, WorkstationUIClass);
}
