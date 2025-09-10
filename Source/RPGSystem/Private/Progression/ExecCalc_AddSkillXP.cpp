// ExecCalc_AddSkillXP.cpp

#include "Progression/ExecCalc_AddSkillXP.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Progression/StatProgressionBridge.h"

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP()
{
}

void UExecCalc_AddSkillXP::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& /*OutExecutionOutput*/) const
{
	const FGameplayEffectSpec& Spec  = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent*   TASC  = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent*   SASC  = ExecutionParams.GetSourceAbilitySystemComponent();

	// Find a stat provider (Target first, then Source).
	UObject* Provider = nullptr;
	if (TASC && TASC->GetOwner())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(TASC->GetOwner());
	}
	if (!Provider && SASC && SASC->GetOwner())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(SASC->GetOwner());
	}
	if (!Provider)
	{
		return; // nowhere to write XP
	}

	// Read the XP delta (SetByCaller). UE 5.2+ API:
	const float XPDelta = Spec.GetSetByCallerMagnitude(SBC.XPDeltaTag, /*Default*/0.f);
	if (FMath::IsNearlyZero(XPDelta))
	{
		return; // nothing to add
	}

	// Which stat to add into? (configured on the calc asset)
	const FGameplayTag SkillXPTag = SBC.SkillStatTag;
	if (!SkillXPTag.IsValid())
	{
		return; // misconfigured calc asset
	}

	// Add XP using your stat bridge (3-arg version).
	UStatProgressionBridge::AddToStat(Provider, SkillXPTag, XPDelta);
}
