#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Crafting/CraftingTypes.h"
#include "CraftingRecipeDataAsset.generated.h"

class UCheckDefinition;
class UXPGrantBundle;

UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	FGameplayTag RecipeIDTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipe")
	FGameplayTag Discipline;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Unlocks")
	FGameplayTag UnlockTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stations")
	FGameplayTagContainer RequiredStationTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::CrafterMustRemain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.01"))
	float CraftSeconds = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items")
	TArray<FCraftItemCost> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Items")
	TArray<FCraftItemOutput> Outputs;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Skill")
	TObjectPtr<UCheckDefinition> CheckDef = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="XP", meta=(AllowedClasses="XPGrantBundle"))
	TSoftObjectPtr<UXPGrantBundle> XPGain;
};
