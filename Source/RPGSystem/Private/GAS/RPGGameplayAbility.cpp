// Source/RPGSystem/Private/GAS/RPGGameplayAbility.cpp
#include "GAS/RPGGameplayAbility.h"
#include "GAS/RPGAbilitySystemComponent.h"

URPGAbilitySystemComponent* URPGGameplayAbility::GetRPGASC() const
{
	return Cast<URPGAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

float URPGGameplayAbility::GA_GetStat(FGameplayTag Tag, float DefaultValue) const
{
	if (const URPGAbilitySystemComponent* ASC = GetRPGASC())
	{
		return ASC->GetStat(Tag, DefaultValue);
	}
	return DefaultValue;
}

bool URPGGameplayAbility::GA_ApplyEffect_SetByCaller(UGameplayEffect* Effect, FGameplayTag MagnitudeTag, float Magnitude, FActiveGameplayEffectHandle& OutHandle) const
{
	OutHandle = FActiveGameplayEffectHandle();
	if (!Effect || !MagnitudeTag.IsValid()) return false;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return false;

	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle     Sh = ASC->MakeOutgoingSpec(Effect->GetClass(), GetAbilityLevel(), Ctx);
	if (!Sh.IsValid()) return false;

	Sh.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);

	OutHandle = ASC->ApplyGameplayEffectSpecToSelf(*Sh.Data.Get());
	return OutHandle.IsValid();
}

bool URPGGameplayAbility::GA_ApplyEffect_FromStat(UGameplayEffect* Effect, FGameplayTag MagnitudeTag, FGameplayTag StatTag, float Scale, float ClampMin, float ClampMax, FActiveGameplayEffectHandle& OutHandle) const
{
	const float Raw = GA_GetStat(StatTag, 0.f);
	const float Mag = FMath::Clamp(Raw * Scale, ClampMin, ClampMax);
	return GA_ApplyEffect_SetByCaller(Effect, MagnitudeTag, Mag, OutHandle);
}
