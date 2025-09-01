#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Crafting/CraftingTypes.h"
#include "CraftingRecipeDataAsset.generated.h"

class UCheckDefinition;
class USkillProgressionData;

/**
 * Data for a single crafting recipe.
 * Keep this *data-only* so designers/modders can add recipes easily.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Optional ID to reference via tags (UI, unlocks, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Identity")
	FGameplayTag RecipeIDTag;

	// How long the craft takes (before any station/player modifiers)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Timing", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.0f;

	// Optional skill check definition to run at start/finish
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Check")
	TObjectPtr<UCheckDefinition> CheckDef = nullptr;

	// Which skill to grant XP in, and how much total XP this recipe yields
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Progression")
	TObjectPtr<USkillProgressionData> SkillForXP = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Progression", meta=(ClampMin="0.0"))
	float XPGain = 0.0f;

	// If >= 0, overrides station default; fraction awarded at start (remainder at finish)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Progression", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float XPOnStartFraction = -1.f;

	// Presence rules for the crafter (must remain near the station, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;

	// Inputs & outputs (re-using the shared crafting structs)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|IO")
	TArray<FCraftItemCost> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe|IO")
	TArray<FCraftItemOutput> Outputs;
};
