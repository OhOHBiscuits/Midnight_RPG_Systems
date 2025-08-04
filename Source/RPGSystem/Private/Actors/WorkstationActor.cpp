#include "Actors/WorkstationActor.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AWorkstationActor::AWorkstationActor()
{	
	bUseEfficiency = true; // Workstations use efficiency!
}

void AWorkstationActor::Interact_Implementation(AActor* Interactor)
{
	Super::Interact_Implementation(Interactor);
	if (!HasAuthority())
		return;

	TriggerWorldItemUI(Interactor);
}
		

