#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.h"
#include "CraftingRecipeDataAsset.generated.h"

class UCheckDefinition;
class USkillProgressionData;

UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// An ID for UI/filters; also copied into the request when crafted
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	FGameplayTag RecipeIDTag;

	// What this recipe consumes/produces
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	TArray<FCraftItemCost> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	TArray<FCraftItemOutput> Outputs;

	// Base craft time (will be modified by skill checks, buffs, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.0f;

	// Optional skill check used when crafting this recipe
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Checks")
	TObjectPtr<UCheckDefinition> Check = nullptr;

	// Skill line & XP values
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression")
	TObjectPtr<USkillProgressionData> SkillForXP = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression", meta=(ClampMin="0.0"))
	float XPTotal = 0.0f;

	// Fraction of XP to grant at start (remainder on finish); <0 means "use station default"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Progression", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float XPOnStartFraction = -1.f;

	// Presence rules
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;

	// (Optional) Which stations can run this; if empty, any station may (filtered by game rules/UI)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stations")
	FGameplayTagContainer RequiredStationTags;
};
