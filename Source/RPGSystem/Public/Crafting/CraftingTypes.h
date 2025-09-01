#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.generated.h"

class UItemDataAsset;
class UCheckDefinition;
class USkillProgressionData;

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftItemCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	FGameplayTag ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftItemOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	TObjectPtr<UItemDataAsset> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

UENUM(BlueprintType)
enum class ECraftPresencePolicy : uint8
{
	None,
	RequireAtStart,
	RequireAtFinish,
	RequireStartFinish
};

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftingRequest
{
	GENERATED_BODY()

	// Optional: purely data/UX tag for filtering recipes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	FGameplayTag RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	TArray<FCraftItemCost> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting")
	TArray<FCraftItemOutput> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.0f;

	// Optional skill check
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|Check")
	TObjectPtr<UCheckDefinition> CheckDef = nullptr;

	// XP routing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|XP")
	TObjectPtr<USkillProgressionData> SkillForXP = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|XP", meta=(ClampMin="0.0"))
	float XPGain = 0.0f;

	// 0..1 at start; remainder on finish. <0 uses station default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|XP", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float XPOnStartFraction = -1.f;

	// Defaults to Instigator if unset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|XP")
	TWeakObjectPtr<AActor> XPRecipient;

	// Presence requirement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting|Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;
};

/** Lightweight summary we broadcast on events */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftingJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Crafting")
	FCraftingRequest Request;

	UPROPERTY(BlueprintReadOnly, Category="Crafting")
	float FinalTime = 0.f;
};
