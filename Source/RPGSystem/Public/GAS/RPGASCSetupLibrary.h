// RPGASCSetupLibrary.h
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RPGASCSetupLibrary.generated.h"

class URPGAbilitySystemComponent;
class APlayerState;
class APawn;

/**
 * Small BP helpers to ensure an ASC exists and is initialized.
 */
UCLASS()
class RPGSYSTEM_API URPGASCSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/** Ensure an ASC on any Actor and initialize it. Returns the ASC. */
	UFUNCTION(BlueprintCallable, Category="GAS|Setup", meta=(DefaultToSelf="Owner", AdvancedDisplay="Avatar, ComponentName"))
	static URPGAbilitySystemComponent* EnsureASCOnActor(AActor* Owner, AActor* Avatar = nullptr, FName ComponentName = TEXT("AbilitySystem"));

	/** Ensure an ASC on a PlayerState and initialize it with a Pawn as Avatar (if provided). */
	UFUNCTION(BlueprintCallable, Category="GAS|Setup", meta=(DefaultToSelf="PlayerState", AdvancedDisplay="Pawn, ComponentName"))
	static URPGAbilitySystemComponent* EnsureASCOnPlayerState(APlayerState* PlayerState, APawn* Pawn = nullptr, FName ComponentName = TEXT("AbilitySystem"));
};
