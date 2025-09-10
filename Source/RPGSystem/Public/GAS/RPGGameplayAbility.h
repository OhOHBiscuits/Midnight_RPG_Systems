// RPGGameplayAbility.h
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "RPGGameplayAbility.generated.h"

UCLASS()
class RPGSYSTEM_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetStatOnSelf(const FGameplayTag& Tag, float DefaultValue = 0.f) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetStatOnSelf(const FGameplayTag& Tag, float NewValue, bool bClampToValidRange = true) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddToStatOnSelf(const FGameplayTag& Tag, float Delta, bool bClampToValidRange = true) const;

protected:
	/** Tries avatar, then owner, for a StatProvider (actor or component). */
	UObject* FindProviderForSelf() const;
};
