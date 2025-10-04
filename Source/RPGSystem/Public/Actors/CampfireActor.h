#pragma once

#include "CoreMinimal.h"
#include "Actors/FuelWorkstationActor.h"
#include "CampfireActor.generated.h"

class USphereComponent;

UCLASS()
class RPGSYSTEM_API ACampfireActor : public AFuelWorkstationActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Campfire|Warmth")
	USphereComponent* WarmthSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campfire|Warmth", meta=(ClampMin="0.0"))
	float WarmthRadius = 400.f;

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateWarmthRadius();
};
