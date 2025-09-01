// CraftingRecipeDataAsset.h
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Crafting/CraftingTypes.h"          // <— ADD
#include "CraftingRecipeDataAsset.generated.h"

UCLASS(BlueprintType)
class RPGSYSTEM_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Recipe")
	FGameplayTag RecipeIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Recipe")
	FGameplayTag CraftingDiscipline;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2_Gates")
	FGameplayTagContainer RequiredStationTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2_Gates")
	FGameplayTagContainer RequiredToolTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3_Inputs")
	TArray<FCraftingItemQuantity> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4_Outputs")
	TArray<FCraftingItemQuantity> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5_Timing", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6_Progression")
	TObjectPtr<class USkillProgressionData> PrimarySkillData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6_Progression")
	float BaseSkillXP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6_Progression")
	float BaseCharacterXP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="7_Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="7_Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Crafting-Recipe")
	FGameplayTag UnlockTag;                          

	
	


};
