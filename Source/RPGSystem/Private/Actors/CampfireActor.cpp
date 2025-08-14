#include "Actors/CampfireActor.h" // FIRST include — fixes the IWYU error
#include "Components/SphereComponent.h"
#include "FuelSystem/FuelComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Simple warmth volume (no collision)
	WarmthSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WarmthSphere"));
	WarmthSphere->SetupAttachment(GetRootComponent());
	WarmthSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarmthSphere->SetGenerateOverlapEvents(false);
	WarmthSphere->SetSphereRadius(WarmthRadius);

	// Fuel workstations should use efficiency by default (inherited flag)
	bUseEfficiency = true;
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateWarmthRadius();

	// If your FuelComponent exposes delegates (e.g. OnBurningChanged), you can bind here:
	// if (UFuelComponent* FC = GetFuelComponent())
	// {
	//     FC->OnBurningChanged.AddDynamic(this, &ACampfireActor::OnFuelStateChanged);
	// }
}

void ACampfireActor::HandleInteract_Server(AActor* Interactor)
{
	// Use the shared UI pipeline: this calls base ShowWorldItemUI on the owning client
	OpenWorkstationUIFor(Interactor);
}

void ACampfireActor::UpdateWarmthRadius()
{
	if (WarmthSphere)
	{
		WarmthSphere->SetSphereRadius(WarmthRadius);
	}
}

#if WITH_EDITOR
void ACampfireActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ACampfireActor, WarmthRadius))
	{
		UpdateWarmthRadius();
	}
}
#endif
