#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"               // FGameplayTag
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Stats/StatProviderInterface.h"              // your interface
#include "StatProgressionBridge.generated.h"

/**
 * Central, engine-safe helpers to read/write your stats and to locate an IStatProviderInterface
 * anywhere on a Pawn/Controller/PlayerState/Components chain.
 *
 * All functions are BlueprintCallable so designers and cues/MMCs can use them too.
 */
UCLASS()
class RPGSYSTEM_API UStatProgressionBridge : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Find first object that implements StatProviderInterface on the given root (actor, its components, controller, playerstate, or owner chain). */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static UObject* FindStatProviderOn(AActor* Root);

	/** Safe GetStat: returns DefaultValue if Provider is null or doesn’t implement the interface. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static float GetStat(UObject* Provider, FGameplayTag Tag, float DefaultValue = 0.f);

	/** Safe SetStat: no-op if Provider is null or doesn’t implement the interface. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static void  SetStat(UObject* Provider, FGameplayTag Tag, float NewValue);

	/** Safe AddToStat: no-op if Provider is null or doesn’t implement the interface. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static void  AddToStat(UObject* Provider, FGameplayTag Tag, float Delta);

	/** Simple, data-agnostic levelling helper (optional). Keeps things designer-driven via tags. */
	UFUNCTION(BlueprintCallable, Category="Progression")
	static void ApplyXPAndLevelUp(
		UObject* Provider,
		FGameplayTag LevelTag,
		FGameplayTag XPTag,
		FGameplayTag XPToNextTag,
		float DeltaXP,
		float MinLevel = 0.f,
		float MaxLevel = 9999.f);
};
