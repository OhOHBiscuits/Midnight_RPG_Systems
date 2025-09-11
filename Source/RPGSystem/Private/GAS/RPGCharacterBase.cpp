#include "GAS/RPGCharacterBase.h" // MUST be first include for IWYU

#include "GAS/RPGPlayerState.h"
#include "GAS/RPGAbilitySystemComponent.h"
#include "GameFramework/Controller.h"

ARPGCharacterBase::ARPGCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

URPGAbilitySystemComponent* ARPGCharacterBase::GetRPGASC() const
{
	if (const ARPGPlayerState* PS = Cast<ARPGPlayerState>(GetPlayerState()))
	{
		return PS->GetRPGASC();
	}
	return nullptr;
}

void ARPGCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureASCInitialized(); // server
}

void ARPGCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	EnsureASCInitialized(); // client
}

void ARPGCharacterBase::EnsureASCInitialized()
{
	if (ARPGPlayerState* PS = Cast<ARPGPlayerState>(GetPlayerState()))
	{
		if (URPGAbilitySystemComponent* ASC = PS->GetRPGASC())
		{
			ASC->InitializeForActor(PS, this);
		}
	}
}
