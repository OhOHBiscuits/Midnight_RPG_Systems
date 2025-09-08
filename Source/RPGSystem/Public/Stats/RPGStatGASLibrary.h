#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "RPGStatGASLibrary.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class URPGStatComponent;

UCLASS()
class RPGSYSTEM_API URPGStatGASLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Get a stat from an actor (looks for RPGStatComponent) */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="Stats|GAS", meta=(DefaultToSelf="FromActor"))
	static bool GetStatFromActor(AActor* FromActor, FGameplayTag StatTag, float& OutValue);

	/** Apply GE to Target, setting a SetByCaller magnitude (MagnitudeTag) from the actor's stat * Multiplier. */
	UFUNCTION(BlueprintCallable, Category="Stats|GAS")
	static bool ApplyGE_WithSetByCallerFromStat(UAbilitySystemComponent* SourceASC,
		TSubclassOf<UGameplayEffect> GEClass,
		AActor* SourceActorForStat,
		FGameplayTag MagnitudeTag,
		AActor* TargetActor,
		float Multiplier = 1.f);

	/** Fire a gameplay cue by tag on an ASC (e.g., for VFX/SFX) */
	UFUNCTION(BlueprintCallable, Category="Stats|GAS")
	static void ExecuteCue(UAbilitySystemComponent* ASC, FGameplayTag CueTag);
};
