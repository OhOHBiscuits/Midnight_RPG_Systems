// StatProgressionBridge.cpp

#include "Progression/StatProgressionBridge.h"
#include "Stats/StatProviderInterface.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Components/ActorComponent.h"

bool UStatProgressionBridge::Implements(UObject* Obj)
{
	return Obj && Obj->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass());
}

float UStatProgressionBridge::GetStat(UObject* Provider, const FGameplayTag& Tag, float DefaultValue)
{
	if (!Provider || !Tag.IsValid())
	{
		return DefaultValue;
	}
	if (Implements(Provider))
	{
		return IStatProviderInterface::Execute_GetStat(Provider, Tag, DefaultValue);
	}
	return DefaultValue;
}

void UStatProgressionBridge::SetStat(UObject* Provider, const FGameplayTag& Tag, float NewValue)
{
	if (!Provider || !Tag.IsValid())
	{
		return;
	}
	if (Implements(Provider))
	{
		IStatProviderInterface::Execute_SetStat(Provider, Tag, NewValue);
	}
}

void UStatProgressionBridge::AddToStat(UObject* Provider, const FGameplayTag& Tag, float Delta)
{
	if (!Provider || !Tag.IsValid())
	{
		return;
	}
	if (Implements(Provider))
	{
		IStatProviderInterface::Execute_AddToStat(Provider, Tag, Delta);
	}
}

static UObject* TryActorForProvider(const AActor* A)
{
	if (!A) return nullptr;

	// Actor itself?
	if (UStatProgressionBridge::Implements(const_cast<AActor*>(A)))
	{
		return const_cast<AActor*>(A);
	}

	// Any components?
	TInlineComponentArray<UActorComponent*> Components(A);
	for (UActorComponent* C : Components)
	{
		if (UStatProgressionBridge::Implements(C))
		{
			return C;
		}
	}
	return nullptr;
}

UObject* UStatProgressionBridge::FindStatProviderOn(const AActor* Actor)
{
	if (!Actor) return nullptr;

	// 1) The actor itself or its components
	if (UObject* P = TryActorForProvider(Actor)) return P;

	// 2) Common chains (pawn <-> controller <-> playerstate)
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (UObject* P = TryActorForProvider(Pawn->GetController()))   return P;
		if (UObject* P = TryActorForProvider(Pawn->GetPlayerState()))  return P;
	}
	if (const AController* Ctrl = Cast<AController>(Actor))
	{
		if (UObject* P = TryActorForProvider(Ctrl->GetPawn()))         return P;
		if (UObject* P = TryActorForProvider(Ctrl->PlayerState))       return P;
	}
	if (const APlayerState* PS = Cast<APlayerState>(Actor))
	{
		if (UObject* P = TryActorForProvider(PS->GetOwner()))          return P; // usually controller
	}

	// 3) Owner chain
	if (UObject* P = TryActorForProvider(Actor->GetOwner()))           return P;

	return nullptr;
}
