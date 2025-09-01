#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.generated.h"

class USkillProgressionData;

/** Item+qty referenced by GameplayTag ID (works with your InventoryHelpers). */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftingItemQuantity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FGameplayTag ItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

UENUM(BlueprintType)
enum class ECraftPresencePolicy : uint8
{
	None               UMETA(DisplayName="None"),
	RequireAtStart     UMETA(DisplayName="Require At Start"),
	RequireAtFinish    UMETA(DisplayName="Require At Finish"),
	RequireStartFinish UMETA(DisplayName="Require Start & Finish")
};

/** Canonical request struct used by Station + Ability. */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FCraftingRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	FGameplayTag RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	TArray<FCraftingItemQuantity> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	TArray<FCraftingItemQuantity> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="2_Timing", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="3_Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="3_Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;

	// Progression
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="4_Progression")
	TObjectPtr<USkillProgressionData> SkillForXP = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="4_Progression")
	float XPGain = 0.f;

	/** 0..1 at start; <0 = use station default */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="4_Progression")
	float XPOnStartFraction = -1.f;

	/** Defaults to Instigator if unset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="4_Progression")
	TWeakObjectPtr<AActor> XPRecipient;
};
