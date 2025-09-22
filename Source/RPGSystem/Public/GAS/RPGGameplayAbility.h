// RPGGameplayAbility.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "RPGGameplayAbility.generated.h"

class URPGAbilitySystemComponent;

/**
 * Clean GameplayAbility base with GAS-only helpers.
 */
UCLASS()
class RPGSYSTEM_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	URPGGameplayAbility();

	/** Typed accessor for our owning ASC. */
	UFUNCTION(BlueprintPure, Category="GAS")
	URPGAbilitySystemComponent* GetRPGASC() const;

	/**
	 * Apply a GameplayEffect (that expects one SetByCaller float) to SELF.
	 * (Uses our ASC from ActorInfo.)
	 */
	UFUNCTION(BlueprintCallable, Category="GAS")
	FActiveGameplayEffectHandle ApplyGEToSelf_SetByCaller(
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag                 SetByCallerTag,
		float                        Magnitude,
		float                        EffectLevel = 1.f,
		UObject*                     SourceObject = nullptr,
		int32                        Stacks = 1) const;

	/**
	 * Apply a GameplayEffect (that expects one SetByCaller float) to TARGET actor.
	 * (Instigator/causer inferred from our Owner/Avatar.)
	 */
	UFUNCTION(BlueprintCallable, Category="GAS")
	FActiveGameplayEffectHandle ApplyGEToTarget_SetByCaller(
		AActor*                      TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag                 SetByCallerTag,
		float                        Magnitude,
		float                        EffectLevel = 1.f,
		UObject*                     SourceObject = nullptr,
		int32                        Stacks = 1) const;
};
