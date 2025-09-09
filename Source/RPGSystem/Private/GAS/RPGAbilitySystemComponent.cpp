#include "GAS/RPGAbilitySystemComponent.h"
#include "Progression/StatProgressionBridge.h" // UStatProgressionBridge
#include "GameFramework/Actor.h"

UObject* URPGAbilitySystemComponent::FindBestStatProvider() const
{
	// Prefer the avatar the client sees
	if (AActor* Avatar = GetAvatarActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
		{
			return P;
		}
	}

	// Fall back to the ASC's owner
	if (AActor* OwningActor = GetOwnerActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(OwningActor))
		{
			return P;
		}
	}

	return nullptr;
}
