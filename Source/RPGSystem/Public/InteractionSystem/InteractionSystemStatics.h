#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "InteractionSystemStatics.generated.h"

class UInteractionSystemComponent;
class AActor;

UCLASS()
class RPGSYSTEM_API UInteractionSystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Resolve the InteractionSystemComponent by walking interface/owner/controller/pawn chain. */
	UFUNCTION(BlueprintCallable, Category="Interaction", meta=(DefaultToSelf="From"))
	static UInteractionSystemComponent* GetInteractionSystem(AActor* From);

	/** Convenience: current looked-at/locked target from the resolved system. */
	UFUNCTION(BlueprintPure, Category="Interaction", meta=(DefaultToSelf="From"))
	static AActor* GetLookTarget(AActor* From);
};
