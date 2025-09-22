#pragma once

#include "UObject/Interface.h"
#include "GameFramework/Actor.h"
#include "InteractionSystem/InteractionSystemComponent.h" // ensure complete type
#include "InteractionSystemInterface.generated.h"

/** UInterface wrapper */
UINTERFACE(BlueprintType)
class RPGSYSTEM_API UInteractionSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement on actors/pawns/controllers that can be interacted with OR can expose an InteractionSystem.
 */
class RPGSYSTEM_API IInteractionSystemInterface
{
	GENERATED_BODY()

public:
	/** Handle an interaction initiated by Interactor (usually server-authoritative). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);

	/** Return the interaction system component (usually on Pawn/Character/Controller). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	UInteractionSystemComponent* GetInteractionSystem() const;

	// ---- Default inline implementations (header-only) ----

	/** No-op default; override in C++ or BP as needed. */
	virtual void Interact_Implementation(AActor* /*Interactor*/) {}

	/** Default: find a UInteractionSystemComponent on this actor (if any). */
	virtual UInteractionSystemComponent* GetInteractionSystem_Implementation() const
	{
		if (const UObject* Obj = _getUObject())
		{
			if (const AActor* AsActor = Cast<AActor>(Obj))
			{
				return AsActor->FindComponentByClass<UInteractionSystemComponent>();
			}
		}
		return nullptr;
	}
};
