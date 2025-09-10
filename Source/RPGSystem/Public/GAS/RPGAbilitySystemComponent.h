// RPGAbilitySystemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "RPGAbilitySystemComponent.generated.h"

/** Small helpers so GAS nodes can read/write your custom stat system. */
UCLASS()
class RPGSYSTEM_API URPGAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetStat(const FGameplayTag& Tag, float DefaultValue = 0.f) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetStat(const FGameplayTag& Tag, float NewValue) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddToStat(const FGameplayTag& Tag, float Delta) const;
};
