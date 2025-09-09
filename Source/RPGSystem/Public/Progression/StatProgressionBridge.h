// Source/RPGSystem/Public/Progression/StatProgressionBridge.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StatProgressionBridge.generated.h"

class UXPGrantBundle;
class USkillProgressionData;

/**
 * Minimal glue that routes XP/Level changes through your Stat System
 * using UStatProviderInterface (by GameplayTags).
 *
 * Safe to call from BP or C++.
 */
UCLASS()
class RPGSYSTEM_API UStatProgressionBridge : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Apply a bundle of XP grants to Target (server-authoritative recommended). */
	UFUNCTION(BlueprintCallable, Category="1_Progression|Bridge")
	static bool ApplyXPBundle(AActor* Target, UXPGrantBundle* Bundle);

	/** Apply XP to a single skill on Target. */
	UFUNCTION(BlueprintCallable, Category="1_Progression|Bridge")
	static bool ApplySkillXP(AActor* Target, USkillProgressionData* Skill, float XPGain,
							 float IncrementOverride = -1.f, bool bCarryRemainderOverride = false);

private:
	/** Find any object on the Actor that implements UStatProviderInterface (component or actor). */
	static UObject* FindStatProvider(UObject* Context);

	/** Get stat through interface (returns DefaultValue if not found). */
	static float GetStat(UObject* Provider, const FGameplayTag& Tag, float DefaultValue);

	/** Set stat through interface. */
	static void  SetStat(UObject* Provider, const FGameplayTag& Tag, float NewValue);

	/** Compute next threshold based on linear rule. Always >= 1. */
	static float ComputeNextThreshold(const USkillProgressionData* Skill, float NewLevel, float IncrementOverride);
};
