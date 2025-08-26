#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SkillCheckTypes.h"
#include "SkillCheckBlueprintLibrary.generated.h"

class UAbilitySystemComponent;
class UCheckDefinition;

UCLASS()
class RPGSYSTEM_API USkillCheckBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Server-only. Computes a single check and returns the result.
	UFUNCTION(BlueprintCallable, Category="1_Progression-Checks", meta=(DefaultToSelf="Instigator"))
	static bool ComputeSkillCheck(AActor* Instigator,
								  AActor* Target,
								  UCheckDefinition* CheckDef,
								  const FSkillCheckParams& Params,
								  FSkillCheckResult& OutResult);

private:
	static UAbilitySystemComponent* ResolveASC(AActor* Actor);
	static bool HasAnyTag(UAbilitySystemComponent* ASC, const TArray<FGameplayTag>& Tags);
	static float GetAbilityMod(UAbilitySystemComponent* ASC, const UCheckDefinition* Def);
	static float GetSkillBonus(UAbilitySystemComponent* ASC, const UCheckDefinition* Def);
	static float GetProficiencyBonus(UAbilitySystemComponent* ASC, const UCheckDefinition* Def);
	static float MapTimeMultiplier(float Margin, const UCheckDefinition* Def);
	static ESkillCheckTier ComputeTier(float Margin);
};
