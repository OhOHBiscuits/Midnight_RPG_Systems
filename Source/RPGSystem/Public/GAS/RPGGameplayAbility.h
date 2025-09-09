#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "RPGGameplayAbility.generated.h"

/**
 * Base gameplay ability with helpers to read/write your custom stat system.
 * These are safe in Blueprints and do not require your ASC subclass,
 * but will use it if present for better provider discovery.
 */
UCLASS()
class RPGSYSTEM_API URPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Read a stat from the best provider found on avatar/owner. */
	UFUNCTION(BlueprintPure, Category="RPG|Stats")
	float GetStatOnSelf(const FGameplayTag& Tag, float DefaultValue = 0.f) const;

	/** Add to a stat on the best provider found on avatar/owner. */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	void AddToStatOnSelf(const FGameplayTag& Tag, float Delta, bool bClampToValidRange = true) const;

protected:
	/** Internal helper: find your stat provider from the owning ASC. */
	UObject* FindProviderForSelf() const;
};
