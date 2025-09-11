#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RPGPlayerController.generated.h"

class URPGAbilitySystemComponent;

/** Auto-wires GAS using the PlayerState ASC on possess/replicate */
UCLASS()
class RPGSYSTEM_API ARPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARPGPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Manually force (re)binding from BP if you want. */
	UFUNCTION(BlueprintCallable, Category="RPG|ASC")
	void SetupASCForPawn(APawn* InPawn);

protected:
	// Server: when we possess a pawn
	virtual void OnPossess(APawn* InPawn) override;
	// Client: PS replicated down later, make sure we rebind too
	virtual void OnRep_PlayerState() override;

private:
	void InitializeASC(AActor* OwnerActor, APawn* AvatarPawn);
};
