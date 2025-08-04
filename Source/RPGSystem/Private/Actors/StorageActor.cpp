#include "Actors/StorageActor.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AStorageActor::AStorageActor()
{
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
	bUseEfficiency = true; // Storage uses efficiency if desired
}

void AStorageActor::Interact_Implementation(AActor* Interactor)
{
	Super::Interact_Implementation(Interactor);
	if (!HasAuthority())
		return;

	TriggerWorldItemUI(Interactor);
}
	
