#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "SkillProgressionData.generated.h"

UCLASS(BlueprintType)
class RPGSYSTEM_API USkillProgressionData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Optional label/tag for filtering/analytics (e.g., Skill.Crafting)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	FGameplayTag SkillTag;

	// Attribute triplet for this skill/stat
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	FGameplayAttribute XPAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	FGameplayAttribute LevelAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	FGameplayAttribute XPToNextAttribute;

	// Per-level threshold increment (default +1000)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	float BaseIncrementPerLevel = 1000.f;

	// If true, leftover XP carries across levels; otherwise XP resets to 0 on level up
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Skill")
	bool bCarryRemainder = false;
};
