// RPGGameplayAbility.cpp

#include "GAS/RPGGameplayAbility.h"
#include "GAS/RPGAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"

URPGGameplayAbility::URPGGameplayAbility()
	: Super()
{
	// Nothing special; base class init.
}

URPGAbilitySystemComponent* URPGGameplayAbility::GetRPGASC() const
{
	return Cast<URPGAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

FActiveGameplayEffectHandle URPGGameplayAbility::ApplyGEToSelf_SetByCaller(
	TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag                 SetByCallerTag,
	float                        Magnitude,
	float                        EffectLevel,
	UObject*                     SourceObject,
	int32                        Stacks) const
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	URPGAbilitySystemComponent* ASC = GetRPGASC();
	if (!ASC)
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGEWithSetByCallerToSelf(EffectClass, SetByCallerTag, Magnitude, EffectLevel, SourceObject, Stacks);
}

FActiveGameplayEffectHandle URPGGameplayAbility::ApplyGEToTarget_SetByCaller(
	AActor*                      TargetActor,
	TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag                 SetByCallerTag,
	float                        Magnitude,
	float                        EffectLevel,
	UObject*                     SourceObject,
	int32                        Stacks) const
{
	if (!EffectClass || !TargetActor)
	{
		return FActiveGameplayEffectHandle();
	}

	// Pull reasonable instigator/causer from our current ability context.
	AActor* Instigator  = GetOwningActorFromActorInfo();
	AActor* EffectCauser = GetAvatarActorFromActorInfo();

	return URPGAbilitySystemComponent::ApplyGEWithSetByCallerToTarget(
		TargetActor, EffectClass, SetByCallerTag, Magnitude, EffectLevel, SourceObject, Stacks, Instigator, EffectCauser);
}
