#include "GAS/RPGGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GAS/RPGAbilitySystemComponent.h"      // optional subclass
#include "Progression/StatProgressionBridge.h"  // bridge to your stat system

UObject* URPGGameplayAbility::FindProviderForSelf() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	// Prefer the ASC we’re executing on
	if (UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get())
	{
		// If it’s your subclass, let it pick the provider.
		if (const URPGAbilitySystemComponent* RPGASC = Cast<URPGAbilitySystemComponent>(ASC))
		{
			if (UObject* P = RPGASC->FindBestStatProvider())
			{
				return P;
			}
		}

		// Otherwise: try avatar, then owner via the bridge.
		if (AActor* Avatar = CurrentActorInfo->AvatarActor.Get())
		{
			if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
			{
				return P;
			}
		}
		if (AActor* Owner = CurrentActorInfo->OwnerActor.Get())
		{
			if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Owner))
			{
				return P;
			}
		}
	}

	return nullptr;
}

float URPGGameplayAbility::GetStatOnSelf(const FGameplayTag& Tag, float DefaultValue) const
{
	if (!Tag.IsValid())
	{
		return DefaultValue;
	}

	if (UObject* Provider = FindProviderForSelf())
	{
		return UStatProgressionBridge::GetStat(Provider, Tag, DefaultValue);
	}
	return DefaultValue;
}

void URPGGameplayAbility::AddToStatOnSelf(const FGameplayTag& Tag, float Delta, bool bClampToValidRange) const
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (UObject* Provider = FindProviderForSelf())
	{
		UStatProgressionBridge::AddToStat(Provider, Tag, Delta, bClampToValidRange);
	}
}
