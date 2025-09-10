// ExecCalc_AddSkillXP.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayTagContainer.h"
#include "ExecCalc_AddSkillXP.generated.h"

/** Which SetByCaller tags this calc reads. */
USTRUCT(BlueprintType)
struct FAddSkillXPCalcConfig
{
	GENERATED_BODY()

	/** SetByCaller: how much XP to add (float). Default tag: Data.XPDelta */
	UPROPERTY(EditDefaultsOnly, Category="SetByCaller")
	FGameplayTag XPDeltaTag;

	/** Which stat to add XP into (gameplay tag that identifies your XP stat). */
	UPROPERTY(EditDefaultsOnly, Category="SetByCaller")
	FGameplayTag SkillStatTag;

	FAddSkillXPCalcConfig()
	{
		XPDeltaTag   = FGameplayTag::RequestGameplayTag(FName(TEXT("Data.XPDelta")), /*ErrorIfNotFound*/false);
		SkillStatTag = FGameplayTag(); // set on the calculation asset
	}
};

UCLASS()
class RPGSYSTEM_API UExecCalc_AddSkillXP : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_AddSkillXP();

	UPROPERTY(EditDefaultsOnly, Category="Config")
	FAddSkillXPCalcConfig SBC;

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
