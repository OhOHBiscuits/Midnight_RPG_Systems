// RPGASCSetupLibrary.cpp

#include "GAS/RPGASCSetupLibrary.h"
#include "GAS/RPGAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

URPGAbilitySystemComponent* URPGASCSetupLibrary::EnsureASCOnActor(AActor* Owner, AActor* Avatar, FName ComponentName)
{
	if (!Owner) return nullptr;

	URPGAbilitySystemComponent* ASC = Owner->FindComponentByClass<URPGAbilitySystemComponent>();
	if (!ASC)
	{
		ASC = URPGAbilitySystemComponent::AddTo(Owner, ComponentName);
	}
	if (ASC)
	{
		ASC->InitializeForActor(Owner, Avatar ? Avatar : Owner);
	}
	return ASC;
}

URPGAbilitySystemComponent* URPGASCSetupLibrary::EnsureASCOnPlayerState(APlayerState* PlayerState, APawn* Pawn, FName ComponentName)
{
	if (!PlayerState) return nullptr;

	URPGAbilitySystemComponent* ASC = PlayerState->FindComponentByClass<URPGAbilitySystemComponent>();
	if (!ASC)
	{
		ASC = URPGAbilitySystemComponent::AddTo(PlayerState, ComponentName);
	}
	if (ASC)
	{
		AActor* Avatar = Pawn ? static_cast<AActor*>(Pawn) : static_cast<AActor*>(PlayerState->GetPawn());
		ASC->InitializeForActor(PlayerState, Avatar ? Avatar : static_cast<AActor*>(PlayerState));
	}
	return ASC;
}
