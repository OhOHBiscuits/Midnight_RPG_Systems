#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "CraftingProficiencyInterface.generated.h"

UINTERFACE(BlueprintType)
class UCraftingProficiencyInterface : public UInterface
{
	GENERATED_BODY()
};

class ICraftingProficiencyInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Crafting|Proficiency")
	int32 GetCraftingProficiencyLevel(const FGameplayTag& SkillTag) const;

	// Optional modifier hook if you want more control (e.g., GAS-driven).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Crafting|Proficiency")
	float GetCraftingTimeFactor(const FGameplayTag& SkillTag) const;
};
