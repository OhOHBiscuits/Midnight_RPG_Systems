#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AbilityModProfile.generated.h"

class UCurveFloat;

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FAbilityModEntry
{
	GENERATED_BODY()

	// E.g., Ability.STR / Ability.DEX / Ability.INT ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayTag AbilityTag;

	// Which attribute holds this ability's "level/score" to feed the curve (e.g., STR_Level)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayAttribute AbilityLevelAttribute;

	// Map Level/Score -> Modifier (DnD-like; e.g., +1 every 4 levels)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<UCurveFloat> ModCurve = nullptr;
};

UCLASS(BlueprintType)
class RPGSYSTEM_API UAbilityModProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TArray<FAbilityModEntry> Entries;
};
