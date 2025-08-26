#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectTypes.h"
#include "ProficiencyProfile.generated.h"

class UCurveFloat;

/**
 * CharacterLevel -> Proficiency Bonus curve.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UProficiencyProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	// Character Level attribute (read from ASC to evaluate the curve)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	FGameplayAttribute CharacterLevelAttribute;

	// Level -> Proficiency Bonus (e.g., 1..4=>+2, 5..8=>+3, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Checks")
	TObjectPtr<UCurveFloat> ProficiencyCurve = nullptr;
};
