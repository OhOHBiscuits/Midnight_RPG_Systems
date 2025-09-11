#include "GAS/RPGPlayerState.h" // must be first include

#include "GAS/RPGAbilitySystemComponent.h"
#include "Stats/RPGStatComponent.h"
#include "AbilitySystemComponent.h"

ARPGPlayerState::ARPGPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	AbilitySystem = ObjectInitializer.CreateDefaultSubobject<URPGAbilitySystemComponent>(this, TEXT("AbilitySystem"));
	if (AbilitySystem)
	{
		AbilitySystem->SetIsReplicated(true);
		
		AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	}

	
	StatComponent = ObjectInitializer.CreateDefaultSubobject<URPGStatComponent>(this, TEXT("StatComponent"));
	if (StatComponent)
	{
		StatComponent->SetIsReplicated(true);
	}

	
	SetNetUpdateFrequency(100.f); 
}

UAbilitySystemComponent* ARPGPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}
