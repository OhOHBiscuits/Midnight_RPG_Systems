// Copyright ...
#include "Progression/SkillProgressionData.h"

USkillProgressionData::USkillProgressionData()
{
	// Reasonable defaults so the GE can work even if you forget to change the tag.
	if (!XPDeltaSetByCallerTag.IsValid())
	{
		XPDeltaSetByCallerTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Data.XPDelta")), /*ErrorIfNotFound=*/false);
	}
}

float USkillProgressionData::GetThresholdForLevel(int32 Level) const
{
	if (!XPThresholdCurve)
	{
		return 0.f;
	}

	// Clamp input to curve’s defined range for safety
	const float X = static_cast<float>(FMath::Clamp(Level, MinLevel, MaxLevel));
	return FMath::Max(0.f, XPThresholdCurve->GetFloatValue(X));
}

float USkillProgressionData::ComputeXPToNext(int32 Level) const
{
	return GetThresholdForLevel(Level);
}
