// RPGAbilitySystemComponent.cpp

#include "GAS/RPGAbilitySystemComponent.h"
#include "Progression/StatProgressionBridge.h"
#include "GameFramework/Actor.h"

URPGAbilitySystemComponent::URPGAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void URPGAbilitySystemComponent::InitializeForActor(AActor* InOwnerActor, AActor* InAvatarActor)
{
	if (!InOwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeForActor called with null Owner"));
		return;
	}
	AActor* AvatarToUse = InAvatarActor ? InAvatarActor : InOwnerActor;
	InitAbilityActorInfo(InOwnerActor, AvatarToUse);
}

URPGAbilitySystemComponent* URPGAbilitySystemComponent::AddTo(AActor* Target)
{
	if (!Target) return nullptr;

	if (URPGAbilitySystemComponent* Existing = Target->FindComponentByClass<URPGAbilitySystemComponent>())
	{
		return Existing;
	}

	URPGAbilitySystemComponent* NewASC =
		NewObject<URPGAbilitySystemComponent>(Target, URPGAbilitySystemComponent::StaticClass(), TEXT("RPGAbilitySystemComponent"));
	if (NewASC)
	{
		NewASC->RegisterComponent();
		NewASC->OnComponentCreated();
		NewASC->InitializeForActor(Target, Target);
	}
	return NewASC;
}

bool URPGAbilitySystemComponent::HasRPGASC(const AActor* Target)
{
	return Target && Target->FindComponentByClass<URPGAbilitySystemComponent>() != nullptr;
}

////

float URPGAbilitySystemComponent::GetStat(const FGameplayTag& Tag, float DefaultValue) const
{
	if (!Tag.IsValid()) return DefaultValue;

	if (const AActor* Avatar = GetAvatarActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
		{
			return UStatProgressionBridge::GetStat(P, Tag, DefaultValue);
		}
	}
	if (const AActor* Owner = GetOwnerActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Owner))
		{
			return UStatProgressionBridge::GetStat(P, Tag, DefaultValue);
		}
	}
	return DefaultValue;
}

void URPGAbilitySystemComponent::SetStat(const FGameplayTag& Tag, float NewValue) const
{
	if (!Tag.IsValid()) return;

	if (const AActor* Avatar = GetAvatarActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
		{
			UStatProgressionBridge::SetStat(P, Tag, NewValue);
			return;
		}
	}
	if (const AActor* Owner = GetOwnerActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Owner))
		{
			UStatProgressionBridge::SetStat(P, Tag, NewValue);
		}
	}
}

void URPGAbilitySystemComponent::AddToStat(const FGameplayTag& Tag, float Delta) const
{
	if (!Tag.IsValid()) return;

	if (const AActor* Avatar = GetAvatarActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Avatar))
		{
			UStatProgressionBridge::AddToStat(P, Tag, Delta);
			return;
		}
	}
	if (const AActor* Owner = GetOwnerActor())
	{
		if (UObject* P = UStatProgressionBridge::FindStatProviderOn(Owner))
		{
			UStatProgressionBridge::AddToStat(P, Tag, Delta);
		}
	}
}
