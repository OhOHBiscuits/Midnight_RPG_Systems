#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "RPGPlayerState.generated.h"

class URPGAbilitySystemComponent;

/** PlayerState that owns the ASC (recommended GAS pattern) */
UCLASS()
class RPGSYSTEM_API ARPGPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARPGPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Direct typed access in C++/BP */
	UFUNCTION(BlueprintPure, Category="RPG|ASC")
	URPGAbilitySystemComponent* GetRPGASC() const { return AbilitySystem; }

protected:
	/** The authoritative ASC for this player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RPG|ASC")
	URPGAbilitySystemComponent* AbilitySystem;
};
