// Source/RPGSystem/Public/GAS/RPGGameplayAbility.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "RPGGameplayAbility.generated.h"

class URPGAbilitySystemComponent;

/**
 * Base GA with helpers that make SetByCaller + stats dead simple.
 */
UCLASS(Abstract)
class RPGSYSTEM_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Convenience cast. Returns nullptr if ASC isn’t RPG ASC. */
	UFUNCTION(BlueprintPure, Category="RPG|Ability")
	URPGAbilitySystemComponent* GetRPGASC() const;

	/** Read a stat from our ASC’s provider. */
	UFUNCTION(BlueprintCallable, Category="RPG|Ability|Stats", meta=(DisplayName="Get Stat (Tag)"))
	float GA_GetStat(FGameplayTag Tag, float DefaultValue = 0.f) const;

	/**
	 * Apply GE to self with one SetByCaller magnitude (prediction-safe).
	 * Magnitude is supplied directly.
	 */
	UFUNCTION(BlueprintCallable, Category="RPG|Ability|Effects")
	bool GA_ApplyEffect_SetByCaller(UGameplayEffect* Effect, FGameplayTag MagnitudeTag, float Magnitude, FActiveGameplayEffectHandle& OutHandle) const;

	/**
	 * Apply GE to self with SetByCaller magnitude pulled from a Stat (Tag).
	 * Scale, ClampMin/Max are optional.
	 */
	UFUNCTION(BlueprintCallable, Category="RPG|Ability|Effects")
	bool GA_ApplyEffect_FromStat(UGameplayEffect* Effect, FGameplayTag MagnitudeTag, FGameplayTag StatTag, float Scale = 1.f, float ClampMin = -FLT_MAX, float ClampMax = FLT_MAX, FActiveGameplayEffectHandle& OutHandle) const;
};
