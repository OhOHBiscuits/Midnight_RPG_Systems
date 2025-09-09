#include "Progression/ExecCalc_AddSkillXP.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayTagContainer.h"
#include "Progression/StatProgressionBridge.h" // UStatProgressionBridge

struct FAddSkillXPStatics
{
	// Tag of the stat that will store XP for the skill.
	// Change to whatever your data uses (or drive from GE if you prefer).
	FGameplayTag SkillStatTag;

	// Optional SetByCaller to configure XP amount on the GE.
	FGameplayTag XPDeltaTag;

	FAddSkillXPStatics()
	{
		SkillStatTag = FGameplayTag::RequestGameplayTag(FName("Gameplay.Stat.Skill.Crafting.XP"));
		XPDeltaTag   = FGameplayTag::RequestGameplayTag(FName("SetByCaller.XPDelta"));
	}
};

static FAddSkillXPStatics& AddSkillXPStatics()
{
	static FAddSkillXPStatics S;
	return S;
}

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP()
{
	// purely tag/stat driven; no captured attributes
}

void UExecCalc_AddSkillXP::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

	// Prefer the target to receive XP
	UObject* Provider = nullptr;

	if (TargetASC && TargetASC->GetOwnerActor())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(TargetASC->GetOwnerActor());
	}
	if (!Provider && SourceASC && SourceASC->GetOwnerActor())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(SourceASC->GetOwnerActor());
	}

	if (!Provider)
	{
		return; // No stat provider available, nothing to do.
	}

	const FAddSkillXPStatics& S = AddSkillXPStatics();

	// Read XP from SetByCaller if present, otherwise default to 1.
	float XPDelta = 1.f;

	// On some engine versions, TryGetSetByCallerMagnitude isn’t available.
	// GetSetByCallerMagnitude(...) returns 0 if not set (with WarnIfNotFound=false).
	{
		// Ensure we see the full type (GameplayEffect.h is already included above).
		const float Maybe = Spec.GetSetByCallerMagnitude(S.XPDeltaTag, /*WarnIfNotFound=*/false);
		if (Maybe != 0.f)
		{
			XPDelta = Maybe;
		}
	}

	// Which stat to add XP into?
	const FGameplayTag SkillXPTag = S.SkillStatTag;
	if (SkillXPTag.IsValid())
	{
		UStatProgressionBridge::AddToStat(Provider, SkillXPTag, XPDelta, /*bClampToValidRange=*/true);
	}
}
