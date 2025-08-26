#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "ProgressionBlueprintLibrary.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class USkillProgressionData;
class UXPGrantBundle;

UCLASS()
class RPGSYSTEM_API UProgressionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Apply one XP award for a given skill/stat. Must run on the server. */
	UFUNCTION(BlueprintCallable, Category="1_Progression-Apply", meta=(DefaultToSelf="Instigator"))
	static bool ApplySkillXP_GE(AActor* Instigator,
								AActor* Target,
								TSubclassOf<UGameplayEffect> AddXPEffectClass,
								USkillProgressionData* SkillData,
								float XPGain,
								float IncrementOverride = -1.f,
								bool bCarryRemainderOverride = false);

	/** Apply many XP awards in one call (e.g., Character + Woodcutting + Strength). */
	UFUNCTION(BlueprintCallable, Category="1_Progression-Apply", meta=(DefaultToSelf="Instigator"))
	static int32 ApplySkillXP_Bundle(AActor* Instigator,
									 AActor* Target,
									 TSubclassOf<UGameplayEffect> AddXPEffectClass,
									 UXPGrantBundle* Bundle);

private:
	/** Resolves the ASC from Actor / PlayerState / Pawn / Controller. */
	static UAbilitySystemComponent* ResolveASCFromActor(AActor* Actor);
};
