#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StatSetDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FScalarDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DefaultValue = 0.f;
};

USTRUCT(BlueprintType)
struct FVitalDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float DefaultCurrent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float Max = 100.f;
};

USTRUCT(BlueprintType)
struct FSkillDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	int32 DefaultLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float StartingXP = 0.f;

	// If <=0, component computes XP to next using its rule.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0"))
	float XPToNextOverride = 0.f;
};

UCLASS(BlueprintType)
class UStatSetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	TArray<FScalarDef> Scalars;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	TArray<FVitalDef> Vitals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	TArray<FSkillDef> Skills;
};
