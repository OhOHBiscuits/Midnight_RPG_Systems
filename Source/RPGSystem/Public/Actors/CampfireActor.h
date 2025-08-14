#pragma once

#include "CoreMinimal.h"
#include "Actors/FuelWorkstationActor.h"
#include "CampfireActor.generated.h"

class USphereComponent;

/**
 * Simple campfire workstation:
 * - Uses inherited Fuel/Byproduct inventories and FuelComponent from AFuelWorkstationActor
 * - Opens the workstation UI via the shared base UI flow
 * - Adds an editor-visible warmth sphere (no collision) you can hook to GAS later
 */
UCLASS()
class RPGSYSTEM_API ACampfireActor : public AFuelWorkstationActor
{
	GENERATED_BODY()

public:
	ACampfireActor();

protected:
	// Visual/Gameplay helper for warmth area (client-side or for overlap queries)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Campfire|Warmth")
	USphereComponent* WarmthSphere;

	// Tweak radius in editor; sphere updates in editor and at runtime
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campfire|Warmth", meta=(ClampMin="0.0"))
	float WarmthRadius = 400.f;

	// --- AActor
	virtual void BeginPlay() override;

	// --- ABaseWorldItemActor: server-side interaction handling
	virtual void HandleInteract_Server(AActor* Interactor) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateWarmthRadius();
};
