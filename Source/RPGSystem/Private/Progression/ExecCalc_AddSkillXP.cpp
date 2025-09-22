#include "Progression/ExecCalc_AddSkillXP.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP()
{
	// You can change this in the asset if you want (e.g., "Data.Progression.XP")
	XPSetByCallerTag = FGameplayTag::RequestGameplayTag(TEXT("Data.XP"));
}

float UExecCalc_AddSkillXP::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// In UE5, FGameplayEffectSpec exposes GetSetByCallerMagnitude(Tag, bWarnIfNotFound)
	// Returns 0 if not found (and can optionally warn).
	const bool bWarnIfNotFound = false;
	const float XP = Spec.GetSetByCallerMagnitude(XPSetByCallerTag, bWarnIfNotFound);

	// You can clamp or scale here if desired.
	return XP;
}
