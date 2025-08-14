// InteractionInterface.h
#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h" // <-- required
#include "UObject/Interface.h"
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FGameplayTag GetInteractionTypeTag() const;
};