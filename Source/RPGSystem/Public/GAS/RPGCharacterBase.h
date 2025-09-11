#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RPGCharacterBase.generated.h"

class URPGAbilitySystemComponent;

/**
 * Minimal Character base that pulls the ASC from the PlayerState and initializes it.
 * Works whether PS is spawned first (server) or replicated later (client).
 */
UCLASS()
class RPGSYSTEM_API ARPGCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ARPGCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Get the owner's RPG ASC (assumes it lives on PlayerState) */
	UFUNCTION(BlueprintPure, Category="RPG|ASC")
	URPGAbilitySystemComponent* GetRPGASC() const;

protected:
	// Server: when we are possessed
	virtual void PossessedBy(AController* NewController) override;

	// Client: when PlayerState replicates
	virtual void OnRep_PlayerState() override;

private:
	void EnsureASCInitialized();
};
