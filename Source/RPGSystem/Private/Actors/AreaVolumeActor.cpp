#include "Actors/AreaVolumeActor.h"
#include "Components/BoxComponent.h"

AAreaVolumeActor::AAreaVolumeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = Box;
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionProfileName(TEXT("OverlapAll"));
}

#if WITH_EDITOR
void AAreaVolumeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
