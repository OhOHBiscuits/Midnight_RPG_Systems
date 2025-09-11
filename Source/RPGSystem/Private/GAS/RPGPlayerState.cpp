#include "GAS/RPGPlayerState.h"
#include "GAS/RPGAbilitySystemComponent.h"

ARPGPlayerState::ARPGPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystem = CreateDefaultSubobject<URPGAbilitySystemComponent>(TEXT("RPGAbilitySystemComponent"));
	// PlayerState replicates; component replication flag is set in the ASC itself.
	NetUpdateFrequency = 100.f;
}

UAbilitySystemComponent* ARPGPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
