#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ExecCalc_AddSkillXP.generated.h"

/**
 * Returns the amount of XP to add as a magnitude, pulled from a SetByCaller tag.
 * Use this inside a GameplayEffect modifier (e.g., Add to Attribute: Skills.XP)
 */
UCLASS()
class RPGSYSTEM_API UExecCalc_AddSkillXP : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_AddSkillXP();

	/** Which SetByCaller tag to read from the effect spec. Default: Data.XP */
	UPROPERTY(EditDefaultsOnly, Category="XP")
	FGameplayTag XPSetByCallerTag;

protected:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
