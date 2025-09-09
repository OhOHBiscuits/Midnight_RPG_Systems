// StatBlueprintLibrary.h
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "StatProviderInterface.h"
#include "StatBlueprintLibrary.generated.h"

UCLASS()
class URPGStatBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Find the first stat provider associated with this actor. */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	static bool FindStatProvider(AActor* Actor, TScriptInterface<IStatProviderInterface>& OutProvider);

	/** Sugar: get stat from whichever provider the actor has. */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	static float GetStatFromActor(AActor* Actor, FGameplayTag Tag, float DefaultValue);

	/** Sugar: set/add via whichever provider exists. */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	static void SetStatOnActor(AActor* Actor, FGameplayTag Tag, float NewValue);

	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	static void AddToStatOnActor(AActor* Actor, FGameplayTag Tag, float Delta);
};
