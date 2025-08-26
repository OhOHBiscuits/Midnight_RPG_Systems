#include "Progression/ExecCalc_AddSkillXP.h"
#include "Progression/SkillProgressionData.h"
#include "Progression/ProgressionTags.h"   // <-- native tags
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP() {}

void UExecCalc_AddSkillXP::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Params,
                                                  FGameplayEffectCustomExecutionOutput& Out) const
{
	const FGameplayEffectSpec& Spec = Params.GetOwningSpec();

	const float XPGain   = Spec.GetSetByCallerMagnitude(TAG_Data_XP, false);
	if (XPGain <= 0.f) return;

	const float SBC_Inc  = Spec.GetSetByCallerMagnitude(TAG_Data_XP_Increment, false);
	const float SBC_Carry= Spec.GetSetByCallerMagnitude(TAG_Data_XP_CarryRemainder, false);

	const UObject* SourceObj = Spec.GetContext().GetSourceObject();
	const USkillProgressionData* Skill = Cast<USkillProgressionData>(SourceObj);
	if (!Skill || !Skill->XPAttribute.IsValid() || !Skill->LevelAttribute.IsValid() || !Skill->XPToNextAttribute.IsValid())
	{
		return;
	}

	const float IncrementPerLevel = (SBC_Inc > 0.f) ? SBC_Inc
		: (Skill->BaseIncrementPerLevel > 0.f ? Skill->BaseIncrementPerLevel : DefaultXPIncrementPerLevel);
	const bool bCarryRemainder = (SBC_Carry > 0.f) ? true
		: (Skill->bCarryRemainder ? true : bDefaultCarryRemainder);

	UAbilitySystemComponent* TargetASC = Params.GetTargetAbilitySystemComponent();
	if (!TargetASC) return;

	float CurrXP     = TargetASC->GetNumericAttribute(Skill->XPAttribute);
	float CurrLevel  = TargetASC->GetNumericAttribute(Skill->LevelAttribute);
	float CurrThresh = TargetASC->GetNumericAttribute(Skill->XPToNextAttribute);
	if (CurrThresh <= 0.f) CurrThresh = IncrementPerLevel;

	const float OrigXP = CurrXP, OrigLevel = CurrLevel, OrigThresh = CurrThresh;

	if (bCarryRemainder)
	{
		float Pool = CurrXP + XPGain;
		while (Pool >= CurrThresh)
		{
			Pool      -= CurrThresh;
			CurrLevel += 1.f;
			CurrThresh += IncrementPerLevel;
		}
		CurrXP = Pool;
	}
	else
	{
		if (CurrXP + XPGain >= CurrThresh)
		{
			CurrLevel += 1.f;
			CurrXP      = 0.f;
			CurrThresh += IncrementPerLevel;
		}
		else
		{
			CurrXP += XPGain;
		}
	}

	const float dXP     = CurrXP    - OrigXP;
	const float dLevel  = CurrLevel - OrigLevel;
	const float dThresh = CurrThresh- OrigThresh;

	if (FMath::Abs(dXP) > KINDA_SMALL_NUMBER)
	{
		Out.AddOutputModifier(FGameplayModifierEvaluatedData(Skill->XPAttribute, EGameplayModOp::Additive, dXP));
	}
	if (FMath::Abs(dLevel) > KINDA_SMALL_NUMBER)
	{
		Out.AddOutputModifier(FGameplayModifierEvaluatedData(Skill->LevelAttribute, EGameplayModOp::Additive, dLevel));
	}
	if (FMath::Abs(dThresh) > KINDA_SMALL_NUMBER)
	{
		Out.AddOutputModifier(FGameplayModifierEvaluatedData(Skill->XPToNextAttribute, EGameplayModOp::Additive, dThresh));
	}
}
