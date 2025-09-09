#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "StatProviderInterface.generated.h"

/** UE interface wrapper */
UINTERFACE(BlueprintType)
class RPGSYSTEM_API UStatProviderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Pure interface (implemented by URPGStatComponent and optionally others).
 * NOTE: Implementations go in *_Implementation methods.
 */
class RPGSYSTEM_API IStatProviderInterface
{
	GENERATED_BODY()

public:
	/** Reads a stat; returns DefaultValue if the tag isn’t found. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Stats")
	float GetStat(FGameplayTag Tag, float DefaultValue) const;

	/** Sets a stat to an absolute value (vitals clamp to [0..Max]). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Stats")
	void  SetStat(FGameplayTag Tag, float NewValue);

	/** Adds Delta to a stat (vitals clamp to [0..Max]). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Stats")
	void  AddToStat(FGameplayTag Tag, float Delta);
};
