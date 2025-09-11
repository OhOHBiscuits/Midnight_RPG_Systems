#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "RPGPlayerState.generated.h"

class URPGAbilitySystemComponent;
class URPGStatComponent;

/**
 * PlayerState that owns the RPG ASC (and optionally our Stat component).
 * Keeps things Blueprint-friendly for quick iteration.
 */
UCLASS()
class RPGSYSTEM_API ARPGPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARPGPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Direct typed accessor for convenience in C++/BP */
	UFUNCTION(BlueprintPure, Category="RPG|ASC")
	URPGAbilitySystemComponent* GetRPGASC() const { return AbilitySystem; }

	/** Optional: expose stats provider if you’re attaching it here */
	UFUNCTION(BlueprintPure, Category="RPG|Stats")
	URPGStatComponent* GetStatComponent() const { return StatComponent; }

protected:
	/** Our ASC lives on PlayerState so it’s always around for possessed/unpossessed swaps */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RPG|ASC")
	TObjectPtr<URPGAbilitySystemComponent> AbilitySystem;

	/** Optional: if you want the stat component on PS (can also live on Pawn/Controller) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RPG|Stats")
	TObjectPtr<URPGStatComponent> StatComponent;
};
