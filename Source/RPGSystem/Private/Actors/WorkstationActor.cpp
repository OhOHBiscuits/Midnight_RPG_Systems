#include "Actors/WorkstationActor.h"
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

AWorkstationActor::AWorkstationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	CraftingStation = CreateDefaultSubobject<UCraftingStationComponent>(TEXT("CraftingStation"));
}

void AWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	if (CraftingStation)
	{
		if (!CraftingStation->OnCraftStarted.IsAlreadyBound(this, &AWorkstationActor::HandleCraftStarted))
		{
			CraftingStation->OnCraftStarted.AddDynamic(this, &AWorkstationActor::HandleCraftStarted);
		}
		if (!CraftingStation->OnCraftFinished.IsAlreadyBound(this, &AWorkstationActor::HandleCraftFinished))
		{
			CraftingStation->OnCraftFinished.AddDynamic(this, &AWorkstationActor::HandleCraftFinished);
		}
	}
}

void AWorkstationActor::HandleCraftStarted(const FCraftingJob& /*Job*/)
{
	// TODO: trigger VFX/UI/etc
}

void AWorkstationActor::HandleCraftFinished(const FCraftingJob& /*Job*/, bool /*bSuccess*/)
{
	// TODO: trigger VFX/UI/etc
}

bool AWorkstationActor::StartRecipe(const UCraftingRecipeDataAsset* Recipe)
{
	if (!CraftingStation || !Recipe) return false;
	return CraftingStation->StartCraftFromRecipe(this, Recipe);
}

void AWorkstationActor::CancelActiveCraft()
{
	if (CraftingStation)
	{
		CraftingStation->CancelCraft();
	}
}
