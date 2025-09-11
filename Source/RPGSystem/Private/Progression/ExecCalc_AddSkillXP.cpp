// Copyright ...
#include "Progression/ExecCalc_AddSkillXP.h"
#include "Progression/SkillProgressionData.h"
#include "AbilitySystemComponent.h"

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP()
{
	// No captures required; we read values directly from the target ASC.
}

float UExecCalc_AddSkillXP::GetNumeric(const UAbilitySystemComponent* ASC, const FGameplayAttribute& Attr)
{
	return (ASC && Attr.IsValid()) ? ASC->GetNumericAttribute(Attr) : 0.f;
}

void UExecCalc_AddSkillXP::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                  FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Pull the skill data from the effect context SourceObject (set by your ability before applying the GE).
	const UObject* SourceObj = Spec.GetContext().GetSourceObject();
	const USkillProgressionData* Skill = Cast<USkillProgressionData>(SourceObj);
	if (!Skill)
	{
		// Without skill description we can’t know which attributes to modify.
		return;
	}

	// XP amount to add comes from SetByCaller; if missing, treat as 0.
	float XPDelta = 0.f;
	if (Skill->XPDeltaSetByCallerTag.IsValid())
	{
		// UE5.6: use GetSetByCallerMagnitude(Tag, /*WarnIfNotFound=*/false)
		XPDelta = Spec.GetSetByCallerMagnitude(Skill->XPDeltaSetByCallerTag, /*WarnIfNotFound=*/false);
	}

	if (XPDelta == 0.f)
	{
		return;
	}

	// Read current values
	float Level    = GetNumeric(TargetASC, Skill->LevelAttribute);
	float XP       = GetNumeric(TargetASC, Skill->XPAttribute);
	float XPToNext = GetNumeric(TargetASC, Skill->XPToNextAttribute);

	// If XPToNext is zero/uninitialized, compute from curve for current level
	if (XPToNext <= 0.f)
	{
		XPToNext = Skill->ComputeXPToNext(static_cast<int32>(Level));
	}

	// Apply XP and handle level-ups (supports multiple in one grant)
	XP = FMath::Max(0.f, XP + XPDelta);

	int32 IntLevel = FMath::Clamp(static_cast<int32>(Level), Skill->MinLevel, Skill->MaxLevel);
	float Threshold = FMath::Max(0.0001f, XPToNext);

	while (XP >= Threshold && IntLevel < Skill->MaxLevel)
	{
		XP -= Threshold;
		++IntLevel;
		Threshold = FMath::Max(0.0001f, Skill->GetThresholdForLevel(IntLevel));
	}

	// If we hit max level, prevent further XP growth
	if (IntLevel >= Skill->MaxLevel)
	{
		IntLevel = Skill->MaxLevel;
		XP       = 0.f;
		Threshold = 0.f;
	}

	// Write back results using Override ops
	if (Skill->LevelAttribute.IsValid())
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->LevelAttribute, EGameplayModOp::Override, static_cast<float>(IntLevel)));
	}
	if (Skill->XPAttribute.IsValid())
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->XPAttribute, EGameplayModOp::Override, XP));
	}
	if (Skill->XPToNextAttribute.IsValid())
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->XPToNextAttribute, EGameplayModOp::Override, Threshold));
	}
}
