#include "GAS/RPGPlayerController.h"
#include "GAS/RPGPlayerState.h"
#include "GAS/RPGAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"

ARPGPlayerController::ARPGPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ARPGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	InitializeASC(PlayerState, InPawn);
}

void ARPGPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeASC(PlayerState, GetPawn());
}

void ARPGPlayerController::SetupASCForPawn(APawn* InPawn)
{
	InitializeASC(PlayerState, InPawn);
}

void ARPGPlayerController::InitializeASC(AActor* OwnerActor, APawn* AvatarPawn)
{
	if (!OwnerActor || !AvatarPawn) return;

	if (ARPGPlayerState* PS = Cast<ARPGPlayerState>(OwnerActor))
	{
		if (URPGAbilitySystemComponent* ASC = PS->GetRPGASC())
		{
			ASC->InitializeForActor(PS, AvatarPawn);
		}
	}
}
