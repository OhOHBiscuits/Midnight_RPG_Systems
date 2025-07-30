#pragma once

#include "CoreMinimal.h"
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
};
