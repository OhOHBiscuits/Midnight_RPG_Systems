// InteractionInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "InteractionInterface.generated.h"

UINTERFACE(BlueprintType)
class RPGSYSTEM_API UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class RPGSYSTEM_API IInteractionInterface
{
	GENERATED_BODY()

public:
	/** Called when a player wants to interact with this actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	/** What kind of interaction this is (Pickup, Storage, Workstation, etc) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FGameplayTag GetInteractionTypeTag() const;
};
