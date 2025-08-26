#include "Progression/SkillCheckBlueprintLibrary.h"
#include "Progression/CheckDefinition.h"
#include "Progression/AbilityModProfile.h"
#include "Progression/ProficiencyProfile.h"
#include "Progression/SkillProgressionData.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Math/UnrealMathUtility.h"

static float EvalCurve(const UCurveFloat* Curve, float X)
{
	return Curve ? Curve->GetFloatValue(X) : 0.f;
}

UAbilitySystemComponent* USkillCheckBlueprintLibrary::ResolveASC(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
		return ASC;

	if (const AController* C = Cast<AController>(Actor))
	{
		if (APlayerState* PS = C->PlayerState)
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
				return ASC;

		if (APawn* P = C->GetPawn())
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P))
				return ASC;
	}

	if (const APawn* P = Cast<APawn>(Actor))
	{
		if (APlayerState* PS = P->GetPlayerState())
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
				return ASC;
	}

	return nullptr;
}

bool USkillCheckBlueprintLibrary::HasAnyTag(UAbilitySystemComponent* ASC, const TArray<FGameplayTag>& Tags)
{
	if (!ASC || Tags.Num() == 0) return false;
	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);
	for (const FGameplayTag& T : Tags)
	{
		if (Owned.HasTag(T)) return true;
	}
	return false;
}

float USkillCheckBlueprintLibrary::GetAbilityMod(UAbilitySystemComponent* ASC, const UCheckDefinition* Def)
{
	if (!ASC || !Def || !Def->AbilityMods) return 0.f;

	for (const FAbilityModEntry& E : Def->AbilityMods->Entries)
	{
		if (E.AbilityTag == Def->PrimaryAbilityTag && E.AbilityLevelAttribute.IsValid())
		{
			const float Level = ASC->GetNumericAttribute(E.AbilityLevelAttribute);
			return EvalCurve(E.ModCurve, Level);
		}
	}
	return 0.f;
}

float USkillCheckBlueprintLibrary::GetSkillBonus(UAbilitySystemComponent* ASC, const UCheckDefinition* Def)
{
	if (!ASC || !Def || !Def->PrimarySkill) return 0.f;
	const FGameplayAttribute LevelAttr = Def->PrimarySkill->LevelAttribute;
	if (!LevelAttr.IsValid()) return 0.f;

	const float Level = ASC->GetNumericAttribute(LevelAttr);
	return EvalCurve(Def->SkillBonusCurve, Level);
}

float USkillCheckBlueprintLibrary::GetProficiencyBonus(UAbilitySystemComponent* ASC, const UCheckDefinition* Def)
{
	if (!ASC || !Def || !Def->Proficiency || !Def->Proficiency->ProficiencyCurve) return 0.f;

	const FGameplayAttribute CharLevelAttr = Def->Proficiency->CharacterLevelAttribute;
	const float CharLevel = CharLevelAttr.IsValid() ? ASC->GetNumericAttribute(CharLevelAttr) : 1.f;
	const float PB = EvalCurve(Def->Proficiency->ProficiencyCurve, CharLevel);

	FGameplayTagContainer Owned;
	ASC->GetOwnedGameplayTags(Owned);
	const bool bProficient = Def->ProficiencyTag.IsValid() && Owned.HasTag(Def->ProficiencyTag);
	const bool bExpertise  = Def->ExpertiseTag.IsValid() && Owned.HasTag(Def->ExpertiseTag);

	if (bExpertise) return PB * 2.f;
	if (bProficient) return PB;
	return 0.f;
}

float USkillCheckBlueprintLibrary::MapTimeMultiplier(float Margin, const UCheckDefinition* Def)
{
	if (!Def) return 1.f;
	const float InMin  = Def->SlowAtMargin;
	const float InMax  = Def->FastAtMargin;
	const float OutMin = Def->TimeAtWorst;
	const float OutMax = Def->TimeAtBest;
	return FMath::GetMappedRangeValueClamped(FVector2D(InMin, InMax), FVector2D(OutMin, OutMax), Margin);
}

ESkillCheckTier USkillCheckBlueprintLibrary::ComputeTier(float Margin)
{
	if (Margin >= 10.f)  return ESkillCheckTier::GreatSuccess;
	if (Margin >= 0.f)   return ESkillCheckTier::Success;
	if (Margin <= -10.f) return ESkillCheckTier::CriticalFail;
	return ESkillCheckTier::Fail;
}

bool USkillCheckBlueprintLibrary::ComputeSkillCheck(AActor* Instigator,
                                                    AActor* Target,
                                                    UCheckDefinition* CheckDef,
                                                    const FSkillCheckParams& Params,
                                                    FSkillCheckResult& OutResult)
{
	OutResult = FSkillCheckResult{};
	if (!CheckDef) return false;

	AActor* AuthRef = Instigator ? Instigator : Target;
	if (!AuthRef || !AuthRef->HasAuthority()) return false;

	UAbilitySystemComponent* ASC = ResolveASC(Instigator ? Instigator : Target);
	if (!ASC) return false;

	const bool bUseRoll = Params.bUseRollOverride ? Params.bUseRoll : CheckDef->bUseRoll;

	const bool bAdvFromTags = HasAnyTag(ASC, CheckDef->AdvantageTags);
	const bool bDisFromTags = HasAnyTag(ASC, CheckDef->DisadvantageTags);
	const bool bAdvantage   = (Params.bForceAdvantage || bAdvFromTags) && !(Params.bForceDisadvantage || bDisFromTags);
	const bool bDisadvantage= (Params.bForceDisadvantage || bDisFromTags) && !(Params.bForceAdvantage || bAdvFromTags);

	float Roll = bUseRoll ? FMath::RandRange(1, 20) : 10.f;
	if (bUseRoll && (bAdvantage || bDisadvantage))
	{
		const int32 R1 = FMath::RandRange(1, 20);
		const int32 R2 = FMath::RandRange(1, 20);
		Roll = bAdvantage ? FMath::Max(R1, R2) : FMath::Min(R1, R2);
	}

	const float AbilityMod   = GetAbilityMod(ASC, CheckDef);
	const float SkillBonus   = GetSkillBonus(ASC, CheckDef);
	const float ProfBonus    = GetProficiencyBonus(ASC, CheckDef);
	const float ToolBonus    = CheckDef->ToolBonus + Params.ToolBonusOverride;
	const float StationBonus = CheckDef->StationBonus + Params.StationBonusOverride;
	const float Situational  = Params.SituationalBonus;

	const float DC = (Params.DCOverride != TNumericLimits<float>::Lowest()) ? Params.DCOverride : CheckDef->DC;

	const float Total  = Roll + AbilityMod + SkillBonus + ProfBonus + ToolBonus + StationBonus + Situational;
	const float Margin = Total - DC;

	OutResult.Total  = Total;
	OutResult.DC     = DC;
	OutResult.Margin = Margin;
	OutResult.bSuccess = (Margin >= 0.f);
	OutResult.Tier   = ComputeTier(Margin);
	OutResult.TimeMultiplier = MapTimeMultiplier(Margin, CheckDef);
	OutResult.QualityTier    = FMath::Clamp(FMath::FloorToInt(Margin / FMath::Max(1, CheckDef->QualityStep)), 0, 10);

	return true;
}
