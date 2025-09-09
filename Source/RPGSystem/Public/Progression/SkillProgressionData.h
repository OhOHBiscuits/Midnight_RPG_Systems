// Source/RPGSystem/Public/Progression/SkillProgressionData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
// (Optional GAS bits you already had)
#include "GameplayEffectTypes.h"
#include "SkillProgressionData.generated.h"

/**
 * Data-driven skill definition that can plug into either your Stat System
 * (via GameplayTags) or, later, GAS (via GameplayAttributes).
 *
 * For maximum moddability, the bridge in this drop reads/writes using the Tag fields.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API USkillProgressionData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Optional label or “group” tag (e.g., Skill.Crafting) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|Skill")
	FGameplayTag SkillTag;

	/** Stat System mapping (used by the bridge below) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|StatTags")
	FGameplayTag LevelTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|StatTags")
	FGameplayTag XPTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|StatTags")
	FGameplayTag XPToNextTag;

	/** Base XP needed for the *next* level (simple linear model: Next = BaseIncrementPerLevel * NewLevel) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|Rules")
	float BaseIncrementPerLevel = 1000.f;

	/** If true, leftover XP carries to the next level-up; otherwise XP resets to 0 on level up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|Rules")
	bool bCarryRemainder = false;

	/** Friendly name for UI (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression|UI")
	FText DisplayName;

	// ---------------- Optional: keep your original GAS fields for future use ----------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="(Optional) GAS")
	FGameplayAttribute XPAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="(Optional) GAS")
	FGameplayAttribute LevelAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="(Optional) GAS")
	FGameplayAttribute XPToNextAttribute;
};
