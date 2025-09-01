#include "Actors/WorkstationActor.h"
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

AWorkstationActor::AWorkstationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CraftingStation = CreateDefaultSubobject<UCraftingStationComponent>(TEXT("CraftingStation"));
}

void AWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	if (CraftingStation)
	{
		if (!CraftingStation->OnCraftFinished.IsAlreadyBound(this, &AWorkstationActor::HandleCraftFinished))
		{
			CraftingStation->OnCraftFinished.AddDynamic(this, &AWorkstationActor::HandleCraftFinished);
		}
	}
}

bool AWorkstationActor::StartRecipe(UCraftingRecipeDataAsset* Recipe, int32 /*Quantity*/)
{
	if (!CraftingStation || !Recipe) return false;
	return CraftingStation->StartCraftFromRecipe(this, Recipe);
}

void AWorkstationActor::OpenWorkstationUIFor(AActor* /*Interactor*/)
{
	// Default: do nothing. Derived classes can open widgets/HUD, play SFX, etc.
}

void AWorkstationActor::HandleCraftFinished(const FCraftingJob& /*Job*/, bool /*bSuccess*/)
{
	// Toggle VFX/SFX here if desired.
}
