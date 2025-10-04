#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "PickupItemActor.generated.h"

class UItemDataAsset;
class UUserWidget;

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

	// Optional small toast shown to the picker
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|UI")
	TSubclassOf<UUserWidget> PickupToastWidgetClass;

	// Decay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup|Decay")
	bool bEnableDecay = true;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Pickup|Decay")
	float DecayTimeRemaining = -1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup|Decay")
	float TotalDecayTime = -1.f;

	UPROPERTY(ReplicatedUsing=OnRep_DecayState, VisibleAnywhere, BlueprintReadOnly, Category="Pickup|Decay")
	EPickupDecayState DecayState = EPickupDecayState::Fresh;

	UPROPERTY(BlueprintAssignable, Category="Pickup|Decay")
	FOnPickupDecayed OnPickupDecayed;

protected:
	virtual void BeginPlay() override;
	virtual void HandleInteract_Server(AActor* Interactor) override;

	// Decay helpers
	void StartDecayTimer();
	void DecayTimerTick();
	void HandleDecayComplete();

	UFUNCTION()
	void OnRep_DecayState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	FTimerHandle DecayTimerHandle;
};
