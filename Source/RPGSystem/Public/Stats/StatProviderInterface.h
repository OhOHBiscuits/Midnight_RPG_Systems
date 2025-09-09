// StatProviderInterface.h
#pragma once

#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "StatProviderInterface.generated.h"

UINTERFACE(BlueprintType)
class UStatProviderInterface : public UInterface
{
	GENERATED_BODY()
};

/** Implement this on a component (or actor) that owns stats */
class IStatProviderInterface
{
	GENERATED_BODY()

public:
	/** Pure read; safe on client. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="RPG|Stats")
	float GetStat(FGameplayTag Tag, float DefaultValue /*=0.f*/) const;

	/** Server-authoritative; implementer should route RPC as needed. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="RPG|Stats")
	void SetStat(FGameplayTag Tag, float NewValue);

	/** Server-authoritative; implementer should route RPC as needed. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="RPG|Stats")
	void AddToStat(FGameplayTag Tag, float Delta);
};
