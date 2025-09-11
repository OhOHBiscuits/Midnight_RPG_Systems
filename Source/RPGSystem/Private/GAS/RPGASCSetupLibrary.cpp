#include "GAS/RPGASCSetupLibrary.h"
#include "GAS/RPGAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

URPGAbilitySystemComponent* URPGASCSetupLibrary::EnsureASCInitialized(AActor* Owner, AActor* Avatar)
{
	if (!Owner) return nullptr;
	URPGAbilitySystemComponent* ASC = URPGAbilitySystemComponent::AddTo(Owner);
	if (ASC)
	{
		ASC->InitializeForActor(Owner, Avatar ? Avatar : Owner);
	}
	return ASC;
}

URPGAbilitySystemComponent* URPGASCSetupLibrary::SetupFromPlayerState(APlayerState* PlayerState, APawn* Pawn)
{
	if (!PlayerState) return nullptr;
	URPGAbilitySystemComponent* ASC = PlayerState->FindComponentByClass<URPGAbilitySystemComponent>();
	if (!ASC)
	{
		ASC = URPGAbilitySystemComponent::AddTo(PlayerState);
	}
	if (ASC)
	{
		ASC->InitializeForActor(PlayerState, Pawn ? Pawn : PlayerState->GetPawn());
	}
	return ASC;
}
