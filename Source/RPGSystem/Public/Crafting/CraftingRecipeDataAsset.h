#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Curves/CurveFloat.h"
#include "CraftingRecipeDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FCraftingItemRequirement
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory|Crafting") FGameplayTag ItemID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory|Crafting", meta=(ClampMin="1")) int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FCraftingItemResult
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory|Crafting") FGameplayTag ItemID;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory|Crafting", meta=(ClampMin="1")) int32 Quantity = 1;
};

UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") FGameplayTag RecipeID;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") TArray<FCraftingItemRequirement> Inputs;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") TArray<FCraftingItemResult> Outputs;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting", meta=(ClampMin="0.0")) float CraftSeconds = 3.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") bool bRequiresFuel = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") FGameplayTagContainer RequiredTags;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting") FGameplayTag StationTypeTag;

	// Proficiency requirements
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting|Proficiency")
	FGameplayTag RequiredSkillTag; // e.g., Crafting.Blacksmithing

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting|Proficiency", meta=(ClampMin="0"))
	int32 RequiredSkillLevel = 0;

	// Optional modifiers driven by level (X = current level, Y = factor)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting|Proficiency")
	UCurveFloat* ProficiencyToTimeFactor = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting|Proficiency")
	UCurveFloat* ProficiencyToYieldFactor = nullptr;
};
