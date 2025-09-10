// GAS/RPGMMC_StatTag.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GameplayTagContainer.h"
#include "RPGMMC_StatTag.generated.h"

/**
 * Reads a numeric value from your custom Stat system (via UStatProgressionBridge)
 * and returns it as this MMC’s base magnitude.
 *
 * Add this MMC to a GameplayEffect and set StatToRead to the tag you want.
 */
UCLASS()
class RPGSYSTEM_API URPGMMC_StatTag : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	URPGMMC_StatTag();

	/** Which stat tag to read from the provider. */
	UPROPERTY(EditDefaultsOnly, Category="Config")
	FGameplayTag StatToRead;

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
