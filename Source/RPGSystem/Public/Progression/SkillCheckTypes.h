#pragma once

#include "CoreMinimal.h"
#include "SkillCheckTypes.generated.h"

UENUM(BlueprintType)
enum class ESkillCheckTier : uint8
{
	CriticalFail UMETA(DisplayName="Critical Fail"),
	Fail         UMETA(DisplayName="Fail"),
	Success      UMETA(DisplayName="Success"),
	GreatSuccess UMETA(DisplayName="Great Success")
};

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FSkillCheckParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	bool bUseRollOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	bool bUseRoll = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float DCOverride = TNumericLimits<float>::Lowest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float ToolBonusOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float StationBonusOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	bool bForceAdvantage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	bool bForceDisadvantage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float SituationalBonus = 0.f;
};

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FSkillCheckResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float Total = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float DC = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float Margin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	ESkillCheckTier Tier = ESkillCheckTier::Fail;

	// Crafting uses these:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	float TimeMultiplier = 1.f; // <1 faster, >1 slower

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Progression-Checks")
	int32 QualityTier = 0;
};
