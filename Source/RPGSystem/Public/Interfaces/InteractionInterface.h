#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h" // <-- This fixes the undefined FGameplayTag error!
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

UINTERFACE(BlueprintType)
class UInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class IInteractionInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FGameplayTag GetInteractionTypeTag() const;
};
