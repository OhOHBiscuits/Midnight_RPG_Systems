#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RPGASCSetupLibrary.generated.h"

class URPGAbilitySystemComponent;

/** One-node BP helpers to add/initialize the RPG ASC anywhere */
UCLASS()
class RPGSYSTEM_API URPGASCSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Add ASC to Actor if missing and initialize with (Owner, Avatar). */
	UFUNCTION(BlueprintCallable, Category="RPG|ASC")
	static URPGAbilitySystemComponent* EnsureASCInitialized(AActor* Owner, AActor* Avatar);

	/** Typical PlayerState setup: Owner=PlayerState, Avatar=Pawn. */
	UFUNCTION(BlueprintCallable, Category="RPG|ASC")
	static URPGAbilitySystemComponent* SetupFromPlayerState(APlayerState* PlayerState, APawn* Pawn);
};
