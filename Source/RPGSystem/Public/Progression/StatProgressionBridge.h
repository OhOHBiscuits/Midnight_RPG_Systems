// StatProgressionBridge.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "StatProgressionBridge.generated.h"

class AActor;

/**
 * Thin utility layer that lets GAS/other code talk to anything that implements
 * UStatProviderInterface (actors or components).
 */
UCLASS()
class RPGSYSTEM_API UStatProgressionBridge : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns DefaultValue if Provider is null, Tag invalid, or Provider doesn't implement the interface. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static float GetStat(UObject* Provider, const FGameplayTag& Tag, float DefaultValue = 0.f);

	UFUNCTION(BlueprintCallable, Category="Stats")
	static void SetStat(UObject* Provider, const FGameplayTag& Tag, float NewValue);

	UFUNCTION(BlueprintCallable, Category="Stats")
	static void AddToStat(UObject* Provider, const FGameplayTag& Tag, float Delta);

	/** Walks common ownership/possession chains and components to find a StatProvider on/near this actor. */
	UFUNCTION(BlueprintCallable, Category="Stats")
	static UObject* FindStatProviderOn(const AActor* Actor);

	/** Convenience: does this object implement UStatProviderInterface? (not exposed to BP) */
	static bool Implements(UObject* Obj);
};
