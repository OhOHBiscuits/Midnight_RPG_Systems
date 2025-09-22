#include "InteractionSystem/InteractionSystemStatics.h"
#include "InteractionSystem/InteractionSystemComponent.h"
#include "InteractionSystem/Interface/InteractionSystemInterface.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

UInteractionSystemComponent* UInteractionSystemStatics::GetInteractionSystem(AActor* From)
{
	if (!From) return nullptr;

	// 1) If the actor implements the interface, ask it directly.
	if (From->GetClass()->ImplementsInterface(UInteractionSystemInterface::StaticClass()))
	{
		return IInteractionSystemInterface::Execute_GetInteractionSystem(From);
	}

	// 2) Search for a component on this actor.
	if (UInteractionSystemComponent* Direct = From->FindComponentByClass<UInteractionSystemComponent>())
	{
		return Direct;
	}

	// 3) Walk common ownership chains (Pawn <-> Controller).
	if (const APawn* AsPawn = Cast<APawn>(From))
	{
		// Pawn implements interface?
		if (AsPawn->GetClass()->ImplementsInterface(UInteractionSystemInterface::StaticClass()))
		{
			if (UInteractionSystemComponent* Comp = IInteractionSystemInterface::Execute_GetInteractionSystem(const_cast<APawn*>(AsPawn)))
			{
				return Comp;
			}
		}

		// Controller side
		if (AController* Ctrl = AsPawn->GetController())
		{
			if (Ctrl->GetClass()->ImplementsInterface(UInteractionSystemInterface::StaticClass()))
			{
				if (UInteractionSystemComponent* Comp = IInteractionSystemInterface::Execute_GetInteractionSystem(Ctrl))
				{
					return Comp;
				}
			}
			if (UInteractionSystemComponent* C = Ctrl->FindComponentByClass<UInteractionSystemComponent>())
			{
				return C;
			}
		}
	}
	else if (const AController* AsCtrl = Cast<AController>(From))
	{
		// Controller implements interface?
		if (AsCtrl->GetClass()->ImplementsInterface(UInteractionSystemInterface::StaticClass()))
		{
			if (UInteractionSystemComponent* Comp = IInteractionSystemInterface::Execute_GetInteractionSystem(const_cast<AController*>(AsCtrl)))
			{
				return Comp;
			}
		}
		// Check possessed pawn
		if (APawn* Pawn = AsCtrl->GetPawn())
		{
			if (Pawn->GetClass()->ImplementsInterface(UInteractionSystemInterface::StaticClass()))
			{
				if (UInteractionSystemComponent* Comp = IInteractionSystemInterface::Execute_GetInteractionSystem(Pawn))
				{
					return Comp;
				}
			}
			if (UInteractionSystemComponent* C = Pawn->FindComponentByClass<UInteractionSystemComponent>())
			{
				return C;
			}
		}
	}

	// 4) Fallback: climb owner chain (useful for components/child actors).
	if (AActor* Owner = From->GetOwner())
	{
		return GetInteractionSystem(Owner);
	}

	return nullptr;
}

AActor* UInteractionSystemStatics::GetLookTarget(AActor* From)
{
	if (UInteractionSystemComponent* Sys = GetInteractionSystem(From))
	{
		return Sys->GetCurrentTarget();
	}
	return nullptr;
}
