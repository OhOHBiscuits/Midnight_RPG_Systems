#include "Actors/WorkstationActor.h"
#include "Blueprint/UserWidget.h"

#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

AWorkstationActor::AWorkstationActor()
{
	// Visuals
	VisualsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualsRoot"));
	VisualsRoot->SetupAttachment(GetRootComponent());

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(VisualsRoot);
	PreviewMesh->SetMobility(EComponentMobility::Movable);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetGenerateOverlapEvents(false);
	PreviewMesh->SetHiddenInGame(true);

	// Crafting
	CraftingStation = CreateDefaultSubobject<UCraftingStationComponent>(TEXT("CraftingStation"));	

	if (CraftingStation) CraftingStation->SetIsReplicated(true);	

	// Workstations typically care about efficiency for capacity/cost scaling.
	bUseEfficiency = true;
}

void AWorkstationActor::HandleInteract_Server(AActor* Interactor)
{
	OpenWorkstationUIFor(Interactor);
}

void AWorkstationActor::OpenWorkstationUIFor(AActor* Interactor)
{
	if (WorkstationWidgetClass)
	{
		// Uses ABaseWorldItemActor's shared UI pipeline
		ShowWorldItemUI(Interactor, WorkstationWidgetClass);
	}

	// Bind once for quick testing feedback (optional)
	if (CraftingStation)
	{
		if (!CraftingStation->OnCraftStarted.IsAlreadyBound(this, &AWorkstationActor::HandleCraftStarted))
		{
			CraftingStation->OnCraftStarted.AddDynamic(this, &AWorkstationActor::HandleCraftStarted);
		}
		if (!CraftingStation->OnCraftCompleted.IsAlreadyBound(this, &AWorkstationActor::HandleCraftCompleted))
		{
			CraftingStation->OnCraftCompleted.AddDynamic(this, &AWorkstationActor::HandleCraftCompleted);
		}
	}
}

bool AWorkstationActor::StartCraftFromRecipe(UCraftingRecipeDataAsset* Recipe)
{
	if (!CraftingStation) return false;
	return CraftingStation->StartCraftFromRecipe(this, Recipe);
}

int32 AWorkstationActor::EnqueueRecipeBatch(UCraftingRecipeDataAsset* Recipe, int32 Quantity)
{
	if (!CraftingQueue || !Recipe || Quantity <= 0) return 0;
	// Keep this generic: queue owns how to start/drive the station internally.
	return CraftingQueue->EnqueueRecipeFor(this, Recipe, Quantity);
}

void AWorkstationActor::CancelCraft()
{
	if (CraftingStation)
	{
		CraftingStation->CancelCraft();
	}
}

void AWorkstationActor::HandleCraftStarted(const FCraftingRequest& /*Request*/, float /*FinalTime*/, const FSkillCheckResult& /*Check*/)
{
	// Hook for bench-specific VFX/SFX if you want
}

void AWorkstationActor::HandleCraftCompleted(const FCraftingRequest& /*Request*/, const FSkillCheckResult& /*Check*/, bool /*bSuccess*/)
{
	// Hook for bench-specific VFX/SFX if you want
}

void AWorkstationActor::HidePreview()
{
	if (PreviewMesh)
	{
		PreviewMesh->SetStaticMesh(nullptr);
		PreviewMesh->SetHiddenInGame(true);
	}
}
