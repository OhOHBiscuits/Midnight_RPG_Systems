#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "CheckDefinition.generated.h"

class UCurveFloat;
class UAbilityModProfile;
class UProficiencyProfile;
class USkillProgressionData;

/**
 * Per-action/recipe/node skill check definition.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UCheckDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<USkillProgressionData> PrimarySkill = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayTag PrimaryAbilityTag; // looked up in AbilityMods

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<UCurveFloat> SkillBonusCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<UAbilityModProfile> AbilityMods = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<UProficiencyProfile> Proficiency = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayTag ProficiencyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayTag ExpertiseTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	float DC = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	bool bUseRoll = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	float ToolBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	float StationBonus = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TArray<FGameplayTag> AdvantageTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TArray<FGameplayTag> DisadvantageTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks", meta=(ClampMin="-50", ClampMax="50"))
	float FastAtMargin = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks", meta=(ClampMin="0.25", ClampMax="4.0"))
	float TimeAtBest = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks", meta=(ClampMin="-50", ClampMax="50"))
	float SlowAtMargin = -10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks", meta=(ClampMin="0.25", ClampMax="4.0"))
	float TimeAtWorst = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks", meta=(ClampMin="1", ClampMax="10"))
	int32 QualityStep = 5;
};
