#include "Actors/CampfireActor.h"
#include "Components/SphereComponent.h"

ACampfireActor::ACampfireActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WarmthSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WarmthSphere"));
	WarmthSphere->SetupAttachment(GetRootComponent());
	WarmthSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarmthSphere->SetGenerateOverlapEvents(false);
	WarmthSphere->SetSphereRadius(WarmthRadius);
}

void ACampfireActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateWarmthRadius();
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
