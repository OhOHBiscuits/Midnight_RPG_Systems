#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_AddSkillXP.generated.h"

/**
 * Adds XP to a specific Skill/Stat defined by a USkillProgressionData placed in the GE Context's SourceObject.
 * - Reads SetByCaller: Data.XP (required), Data.XP.Increment (optional), Data.XP.CarryRemainder (optional)
 * - Updates three attributes: XP, Level, XPToNext (from the data asset)
 */
UCLASS()
class RPGSYSTEM_API UExecCalc_AddSkillXP : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_AddSkillXP();

	// Fallbacks if the Skill data (or SBC) doesn’t provide them
	UPROPERTY(EditDefaultsOnly, Category="1_Progression-Defaults")
	float DefaultXPIncrementPerLevel = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category="1_Progression-Defaults")
	bool bDefaultCarryRemainder = false;

	// In UE 5.6, override Execute_Implementation (Execute is a BlueprintNativeEvent)
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
										FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
