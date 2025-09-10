// RPGGameplayAbility.cpp

#include "GAS/RPGGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Progression/StatProgressionBridge.h"

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

void URPGGameplayAbility::SetStatOnSelf(const FGameplayTag& Tag, float NewValue, bool /*bClampToValidRange*/) const
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (UObject* Provider = FindProviderForSelf())
	{
		UStatProgressionBridge::SetStat(Provider, Tag, NewValue);
	}
}

void URPGGameplayAbility::AddToStatOnSelf(const FGameplayTag& Tag, float Delta, bool /*bClampToValidRange*/) const
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (UObject* Provider = FindProviderForSelf())
	{
		UStatProgressionBridge::AddToStat(Provider, Tag, Delta);
	}
}

UObject* URPGGameplayAbility::FindProviderForSelf() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return nullptr;

	if (const AActor* Avatar = ASC->GetAvatarActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
		{
			return P;
		}
	}
	if (const AActor* Owner = ASC->GetOwnerActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Owner))
		{
			return P;
		}
	}
	return nullptr;
}
