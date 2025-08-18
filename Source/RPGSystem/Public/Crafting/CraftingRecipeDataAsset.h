#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "CraftingRecipeDataAsset.generated.h"

class UItemDataAsset;
class UAnimMontage;

/** A simple [ItemIDTag, Quantity] pair */
USTRUCT(BlueprintType)
struct FCraftingItemQuantity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	FGameplayTag ItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

/**
 * Data-driven recipe definition. No logic; the Craft Ability reads this.
 * Cook-safe: only GameplayTags + SoftObjectPtrs (for optional montage).
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Unique recipe ID used in UI/logs (optional but nice). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Identity", meta=(Categories="Recipe"))
	FGameplayTag RecipeIDTag;

	/** Tag that must be present on the player ASC to craft this (granted by an infinite GE when learned). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Unlock", meta=(Categories="Recipe.Unlocked"))
	FGameplayTag UnlockTag;

	/** Station/tool gating. AllOf must be present; AnyOf at least one; NoneOf must be absent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Gates", meta=(Categories="Station"))
	FGameplayTagContainer RequiredStationTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Gates", meta=(Categories="Tools"))
	FGameplayTagContainer RequiredToolTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Inputs")
	TArray<FCraftingItemQuantity> Inputs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Outputs")
	TArray<FCraftingItemQuantity> Outputs;

	/** Base craft time for quantity=1 (seconds). Ability scales this by station efficiency & skills later. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Timing", meta=(ClampMin="0.01"))
	float BaseTimeSeconds = 3.0f;

	/** Optional montage for cosmetics; craft still completes without it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Montage")
	TSoftObjectPtr<UAnimMontage> CraftMontage;

	/** Optional: play this section/name if you want (empty = default). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Montage")
	FName MontageSectionName;

	/** Optional: Skill/XP metadata — used by your reward GE/logic later. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Progression", meta=(Categories="Skill"))
	FGameplayTag PrimarySkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Progression")
	float BaseSkillXP = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Recipe|Progression")
	float BaseCharacterXP = 2.f;

	/** Helper for UI: returns true if arrays look valid. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Crafting-Recipe|Query")
	bool IsValidRecipe() const { return Inputs.Num() > 0 && Outputs.Num() > 0 && BaseTimeSeconds > 0.f; }
};
