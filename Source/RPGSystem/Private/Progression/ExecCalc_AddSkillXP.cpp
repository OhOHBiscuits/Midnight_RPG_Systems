// ExecCalc_AddSkillXP.cpp

#include "Progression/ExecCalc_AddSkillXP.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayTagContainer.h"

#include "Progression/SkillProgressionData.h"
#include "Progression/ProgressionTags.h"

// Your custom stat bridge
#include "Stats/StatProviderInterface.h"

namespace
{
	/** Read a SetByCaller magnitude safely (0 if unset). */
	static float GetSBC(const FGameplayEffectSpec& Spec, const FGameplayTag& Tag)
	{
		return Spec.GetSetByCallerMagnitude(Tag, /*WarnIfNotFound*/ false, 0.f);
	}

	/** Try to find any UObject on/around an actor that implements the stat interface. */
	static UObject* FindStatProviderOn(AActor* Root)
	{
		if (!Root) return nullptr;

		// Actor itself?
		if (Root->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
		{
			return Root;
		}

		// Components?
		TArray<UActorComponent*> Comps;
		Root->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (C && C->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
			{
				return C;
			}
		}

		// Owner chain (Controller / PlayerState etc.)
		AActor* Owner = Root->GetOwner();
		if (Owner && Owner != Root)
		{
			if (UObject* P = FindStatProviderOn(Owner))
			{
				return P;
			}
		}

		return nullptr;
	}

	/** Extract stat-bridge tags from the GE asset tags (see doc in header). */
	static void ExtractBridgeTags(const FGameplayEffectSpec& Spec, FGameplayTag& OutXP, FGameplayTag& OutLevel, FGameplayTag& OutThresh)
	{
		OutXP = FGameplayTag();
		OutLevel = FGameplayTag();
		OutThresh = FGameplayTag();

		FGameplayTagContainer All;
		Spec.GetAllAssetTags(All);

		// We parse by string prefix so designers can define any concrete tag after the prefix.
		// eg: "StatBridge.XPTag.Stat.Skill.Blacksmith.XP"
		static const FString XPPrefix(TEXT("StatBridge.XPTag."));
		static const FString LevelPrefix(TEXT("StatBridge.LevelTag."));
		static const FString ThreshPrefix(TEXT("StatBridge.ThresholdTag."));

		auto MaybeParse = [](const FString& Full, const FString& Prefix)->FGameplayTag
		{
			if (!Full.StartsWith(Prefix)) return FGameplayTag();
			const FString Suffix = Full.Mid(Prefix.Len());
			if (Suffix.IsEmpty()) return FGameplayTag();
			// Don't crash if not found; just return invalid.
			return FGameplayTag::RequestGameplayTag(FName(*Suffix), /*ErrorIfNotFound*/ false);
		};

		for (const FGameplayTag& T : All)
		{
			const FString S = T.ToString();
			if (!OutXP.IsValid())
			{
				if (FGameplayTag Parsed = MaybeParse(S, XPPrefix); Parsed.IsValid()) OutXP = Parsed;
			}
			if (!OutLevel.IsValid())
			{
				if (FGameplayTag Parsed = MaybeParse(S, LevelPrefix); Parsed.IsValid()) OutLevel = Parsed;
			}
			if (!OutThresh.IsValid())
			{
				if (FGameplayTag Parsed = MaybeParse(S, ThreshPrefix); Parsed.IsValid()) OutThresh = Parsed;
			}
		}
	}

	/** Round/truncate convenience for readable math. */
	static int32 FloorToIntNonNeg(float V) { return static_cast<int32>(FMath::Max(0.f, FMath::FloorToFloat(V))); }
}

UExecCalc_AddSkillXP::UExecCalc_AddSkillXP()
{
	// No attribute captures needed; we read current values directly from the ASC.
}

void UExecCalc_AddSkillXP::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Data asset that defines the 3 attributes + leveling curve.
	const USkillProgressionData* Skill = Cast<USkillProgressionData>(Spec.Def ? Spec.Def->SourceObject : nullptr);
	if (!Skill)
	{
		return; // nothing to do
	}

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	// Read incoming XP and an optional multiplier via your existing tags.
	float IncomingXP = GetSBC(Spec, TAG_Data_XP);
	const float MultA = GetSBC(Spec, TAG_Mult_A); // optional multiplier (designer-driven)
	const float MultB = GetSBC(Spec, TAG_Mult_B); // optional multiplier (designer-driven)

	if (MultA != 0.f) IncomingXP *= MultA;
	if (MultB != 0.f) IncomingXP *= MultB;

	IncomingXP = FMath::Max(0.f, IncomingXP);

	// Pull current GAS values
	const float CurXP     = TargetASC->GetNumericAttribute(Skill->XPAttribute);
	const float CurLevelF = TargetASC->GetNumericAttribute(Skill->LevelAttribute);
	const float CurThresh = TargetASC->GetNumericAttribute(Skill->XPToNextAttribute);

	int32  Level     = FMath::Max(0, static_cast<int32>(CurLevelF));
	float  XP        = FMath::Max(0.f, CurXP);
	float  Threshold = FMath::Max(0.0001f, CurThresh > 0.f ? CurThresh : Skill->GetThresholdForLevel(Level));

	// Apply incoming XP and roll over levels
	XP += IncomingXP;

	int32 LevelsGained = 0;
	while (XP >= Threshold)
	{
		XP -= Threshold;
		Level++;
		LevelsGained++;
		Threshold = FMath::Max(0.0001f, Skill->GetThresholdForLevel(Level));
	}

	// If we don't carry remainder, push leftover XP to next threshold or zero (your asset controls this)
	if (!Skill->bCarryRemainder && LevelsGained > 0)
	{
		XP = 0.f;
	}

	// Write back to GAS via output modifiers
	const float NewLevelF = static_cast<float>(Level);
	const float DeltaLevel = NewLevelF - CurLevelF;
	const float DeltaXP    = XP - CurXP;
	const float DeltaThresh= Threshold - CurThresh;

	if (!FMath::IsNearlyZero(DeltaXP))
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->XPAttribute, EGameplayModOp::Additive, DeltaXP));
	}
	if (!FMath::IsNearlyZero(DeltaLevel))
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->LevelAttribute, EGameplayModOp::Additive, DeltaLevel));
	}
	// Threshold is usually set (not added). Use Override if your build supports it, else "Additive to difference".
	if (!FMath::IsNearlyZero(DeltaThresh))
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(Skill->XPToNextAttribute, EGameplayModOp::Additive, DeltaThresh));
	}

	// -------------------------------
	// Optional: Mirror into Stat System (data-driven, zero C++ changes)
	// -------------------------------
	FGameplayTag XPTag, LevelTag, ThreshTag;
	ExtractBridgeTags(Spec, XPTag, LevelTag, ThreshTag);

	if (XPTag.IsValid() || LevelTag.IsValid() || ThreshTag.IsValid())
	{
		AActor* TargetActor = TargetASC->GetAvatarActor();
		UObject* Provider = FindStatProviderOn(TargetActor);
		if (!Provider) // try owner chain of ASC if no avatar
		{
			if (AActor* Owner = TargetASC->GetOwnerActor())
			{
				Provider = FindStatProviderOn(Owner);
			}
		}

		if (Provider && Provider->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
		{
			// Read existing (with defaults)
			float OldXP = XPTag.IsValid() ? IStatProviderInterface::Execute_GetStat(Provider, XPTag, 0.f) : 0.f;
			float OldLv = LevelTag.IsValid() ? IStatProviderInterface::Execute_GetStat(Provider, LevelTag, 0.f) : CurLevelF;
			float OldTh = ThreshTag.IsValid() ? IStatProviderInterface::Execute_GetStat(Provider, ThreshTag, CurThresh) : CurThresh;

			// New values computed above: XP, NewLevelF, Threshold
			if (XPTag.IsValid() && !FMath::IsNearlyEqual(OldXP, XP))
			{
				IStatProviderInterface::Execute_SetStat(Provider, XPTag, XP);
			}
			if (LevelTag.IsValid() && !FMath::IsNearlyEqual(OldLv, NewLevelF))
			{
				IStatProviderInterface::Execute_SetStat(Provider, LevelTag, NewLevelF);
			}
			if (ThreshTag.IsValid() && !FMath::IsNearlyEqual(OldTh, Threshold))
			{
				IStatProviderInterface::Execute_SetStat(Provider, ThreshTag, Threshold);
			}
		}
	}
}
