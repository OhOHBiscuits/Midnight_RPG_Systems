#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RPGCharacterBase.generated.h"

class URPGAbilitySystemComponent;

/** Minimal Character base with a helper to fetch the ASC from PlayerState */
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
	// Client re-bind when PS finishes replicating
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

private:
	void EnsureASCInitialized();
};
