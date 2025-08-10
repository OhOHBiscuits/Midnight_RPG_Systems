#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "PickupItemActor.generated.h"

UENUM(BlueprintType)
enum class EPickupDecayState : uint8
{
	Fresh,
	Decayed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPickupDecayed);

UCLASS()
class RPGSYSTEM_API APickupItemActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	APickupItemActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	int32 Quantity = 1;

	virtual void Interact_Implementation(AActor* Interactor) override;

	// ---- Decay Additions ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	bool bEnableDecay = true;

	// Replicated for UI/clients (progress bar etc)
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	float DecayTimeRemaining = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	float TotalDecayTime = -1.f;

	FTimerHandle DecayTimerHandle;

	// For multiplayer: replicated decay state for mesh swap/VFX
	UPROPERTY(ReplicatedUsing=OnRep_DecayState, VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	EPickupDecayState DecayState = EPickupDecayState::Fresh;

	// Called when the item decays completely (for BP UI/VFX)
	UPROPERTY(BlueprintAssignable, Category="Pickup")
	FOnPickupDecayed OnPickupDecayed;

protected:
	virtual void BeginPlay() override;
	void StartDecayTimer();
	void DecayTimerTick();
	void HandleDecayComplete();

	// RepNotify handler
	UFUNCTION()
	void OnRep_DecayState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
