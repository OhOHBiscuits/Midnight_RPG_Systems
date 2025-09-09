#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StatSetDataAsset.generated.h"

/** Scalar: single float (e.g., MoveSpeed, CritChance) */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FRPGScalarDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText        Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float        DefaultValue = 0.f;
};

/** Vital: Current/Max pair (e.g., Health, Stamina, Hunger) */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FRPGVitalDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText        Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float        DefaultCurrent = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float        DefaultMax     = 100.f;
};

/** Skill: Level & XP (e.g., Smithing, Cooking) */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FRPGSkillDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText        Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32        DefaultLevel    = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float        DefaultXP       = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float        DefaultXPToNext = 100.f;
};

/** Bundle of stat definitions that designers/mods can author. */
UCLASS(BlueprintType)
class RPGSYSTEM_API UStatSetDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRPGScalarDef> Modifiers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRPGVitalDef>  Vitals;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRPGSkillDef>  Skills;
};
