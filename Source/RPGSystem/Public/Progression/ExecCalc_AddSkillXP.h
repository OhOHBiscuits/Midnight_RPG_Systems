// Source/RPGSystem/Public/Progression/ExecCalc_AddSkillXP.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayTagContainer.h"
#include "ExecCalc_AddSkillXP.generated.h"

UCLASS()
class RPGSYSTEM_API UExecCalc_AddSkillXP : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_AddSkillXP();

	// Which skill stat to add XP to (e.g., RPG.Skill.Crafting)
	UPROPERTY(EditDefaultsOnly, Category="Progression")
	FGameplayTag SkillStatTag;

	// The SetByCaller tag that carries the XP delta (e.g., Progression.XPDelta)
	UPROPERTY(EditDefaultsOnly, Category="Progression")
	FGameplayTag XPDeltaSBC;

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
