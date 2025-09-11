// Copyright ...
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Curves/CurveFloat.h"
#include "SkillProgressionData.generated.h"

UCLASS(BlueprintType)
class RPGSYSTEM_API USkillProgressionData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Attribute that stores the current Skill Level (integer stored as float). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Attributes")
	FGameplayAttribute LevelAttribute;

	/** Attribute that stores the current Skill XP accumulated towards next level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Attributes")
	FGameplayAttribute XPAttribute;

	/** Attribute that stores the current XP threshold to reach the next level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Attributes")
	FGameplayAttribute XPToNextAttribute;

	/** Curve mapping Level -> XP needed to reach next Level (X = Level, Y = required XP). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Progression")
	TObjectPtr<UCurveFloat> XPThresholdCurve = nullptr;

	/** SetByCaller tag used by the granting GameplayEffect to pass the XP delta (default: Data.XPDelta). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Granting")
	FGameplayTag XPDeltaSetByCallerTag;

	/** Optional floor/ceiling for levels (inclusive). LevelUp logic clamps to this range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Progression", meta=(ClampMin="0", UIMin="0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill|Progression", meta=(ClampMin="0", UIMin="0"))
	int32 MaxLevel = 100;

public:
	USkillProgressionData();

	/** Returns the XP required to go from Level -> Level+1. */
	UFUNCTION(BlueprintCallable, Category="Skill|Progression")
	float GetThresholdForLevel(int32 Level) const;

	/** Convenience: (re)initialize XPToNext for a given level from the curve; if curve missing returns 0. */
	UFUNCTION(BlueprintCallable, Category="Skill|Progression")
	float ComputeXPToNext(int32 Level) const;
};
