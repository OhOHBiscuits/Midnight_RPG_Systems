// Copyright ...
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_AddSkillXP.generated.h"

class USkillProgressionData;


UCLASS()
class RPGSYSTEM_API UExecCalc_AddSkillXP : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_AddSkillXP();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	static float GetNumeric(const UAbilitySystemComponent* ASC, const FGameplayAttribute& Attr);
};
