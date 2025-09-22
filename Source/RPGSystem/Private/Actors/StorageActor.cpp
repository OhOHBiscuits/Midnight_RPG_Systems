#include "Actors/StorageActor.h"
#include "Inventory/InventoryComponent.h"
#include "Blueprint/UserWidget.h"

AStorageActor::AStorageActor()
{
	bUseEfficiency = true; // storage can leverage efficiency if desired

	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
	InventoryComp->SetIsReplicated(true);
}

void AStorageActor::BeginPlay()
{
	Super::BeginPlay();
	// Any init that depends on ItemData could go here
}

void AStorageActor::OpenStorageUIFor(AActor* Interactor)
{
	TSubclassOf<UUserWidget> ClassToUse = StorageWidgetClass;
	ShowWorldItemUI(Interactor, ClassToUse);
}

void AStorageActor::HandleInteract_Server(AActor* Interactor)
{
	OpenStorageUIFor(Interactor);
}
