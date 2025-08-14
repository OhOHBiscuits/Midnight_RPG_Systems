#include "Actors/WorkstationActor.h"
#include "Blueprint/UserWidget.h"

AWorkstationActor::AWorkstationActor()
{
	VisualsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualsRoot"));
	VisualsRoot->SetupAttachment(GetRootComponent());

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(VisualsRoot);
	PreviewMesh->SetMobility(EComponentMobility::Movable);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->SetGenerateOverlapEvents(false);
	PreviewMesh->SetHiddenInGame(true);
	
	// Workstations typically care about efficiency for capacity/cost scaling.
	bUseEfficiency = true;
}

void AWorkstationActor::HandleInteract_Server(AActor* Interactor)
{
	// Default behavior: just open the UI for the interacting player
	OpenWorkstationUIFor(Interactor);
}

void AWorkstationActor::OpenWorkstationUIFor(AActor* Interactor)
{
	// Use the shared UI pipeline in the base actor; server will route to owning client.
	if (WorkstationWidgetClass)
	{
		ShowWorldItemUI(Interactor, WorkstationWidgetClass);
	}
}


void AWorkstationActor::HidePreview()
{
	if (PreviewMesh)
	{
		PreviewMesh->SetStaticMesh(nullptr);
		PreviewMesh->SetHiddenInGame(true);
	}
}
