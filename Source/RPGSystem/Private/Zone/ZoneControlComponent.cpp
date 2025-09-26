#include "Zone/ZoneControlComponent.h"
#include "Zone/SmartZoneComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"

#include "EquipmentSystem/EquipmentSystemInterface.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

UZoneControlComponent::UZoneControlComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UZoneControlComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentOwnerFaction = InitialOwnerFaction;
	CurrentAttackerFaction = AttackerFaction;

	if (AActor* O = GetOwner())
	{
		SmartZone = O->FindComponentByClass<USmartZoneComponent>();
	}

	PushModeToSmartZone();
}

void UZoneControlComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UZoneControlComponent, ControlState);
	DOREPLIFETIME(UZoneControlComponent, CurrentOwnerFaction);
	DOREPLIFETIME(UZoneControlComponent, CurrentAttackerFaction);
	DOREPLIFETIME(UZoneControlComponent, CaptureProgress01);
	DOREPLIFETIME(UZoneControlComponent, bInCombat);
}

// --- Rep notifies ---
void UZoneControlComponent::OnRep_ControlState()
{
	OnControlChanged.Broadcast(ControlState, CurrentOwnerFaction);
}
void UZoneControlComponent::OnRep_OwnerFaction()
{
	OnControlChanged.Broadcast(ControlState, CurrentOwnerFaction);
	PushModeToSmartZone();
}
void UZoneControlComponent::OnRep_AttackerFaction()
{
	OnCaptureProgress.Broadcast(CaptureProgress01, CurrentAttackerFaction);
}
void UZoneControlComponent::OnRep_CaptureProgress()
{
	OnCaptureProgress.Broadcast(CaptureProgress01, CurrentAttackerFaction);
}
void UZoneControlComponent::OnRep_Combat()
{
	OnCombatChanged.Broadcast(bInCombat);
	PushModeToSmartZone();
}

// --- Client wrappers ---
void UZoneControlComponent::TryForceLiberateToPlayer()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) Server_ForceLiberateToPlayer(); else Server_ForceLiberateToPlayer(); }
}
void UZoneControlComponent::TryForceOwnerFaction(FGameplayTag NewOwner)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) Server_ForceOwnerFaction(NewOwner); else Server_ForceOwnerFaction(NewOwner); }
}
void UZoneControlComponent::TryForceCombat(bool bCombat)
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) Server_ForceCombat(bCombat); else Server_ForceCombat(bCombat); }
}
void UZoneControlComponent::TryResetZone()
{
	if (AActor* O = GetOwner()){ if (O->HasAuthority()) Server_ResetZone(); else Server_ResetZone(); }
}

// --- Presence ---
APlayerState* UZoneControlComponent::ResolvePS(AActor* Actor) const
{
	if (!Actor) return nullptr;
	if (const APawn* P = Cast<APawn>(Actor)) return P->GetPlayerState();
	if (const AController* C = Cast<AController>(Actor)) return C->PlayerState;
	return Cast<APlayerState>(Actor);
}

bool UZoneControlComponent::IsActorFaction(AActor* Actor, const FGameplayTag& Faction) const
{
	if (!Actor || !Faction.IsValid()) return false;

	if (APlayerState* PS = ResolvePS(Actor))
	{
		if (PS->GetClass()->ImplementsInterface(UEquipmentSystemInterface::StaticClass()))
		{
			if (UAbilitySystemComponent* ASC = IEquipmentSystemInterface::Execute_ES_GetASC(PS))
			{
				if (ASC->HasMatchingGameplayTag(Faction)) return true;
			}
		}
	}
	return false;
}

void UZoneControlComponent::NotifyActorEntered(AActor* Other)
{
	if (!Other) return;
	if (AActor* O = GetOwner(); !O || !O->HasAuthority()) return;

	if (IsActorFaction(Other, CurrentOwnerFaction)) ++NumDefenders;
	if (IsActorFaction(Other, CurrentAttackerFaction)) ++NumAttackers;

	if (APlayerState* PS = ResolvePS(Other))
	{
		PresentPlayers.Add(PS);
	}

	StartCaptureTimer();
}

void UZoneControlComponent::NotifyActorLeft(AActor* Other)
{
	if (!Other) return;
	if (AActor* O = GetOwner(); !O || !O->HasAuthority()) return;

	if (IsActorFaction(Other, CurrentOwnerFaction) && NumDefenders > 0) --NumDefenders;
	if (IsActorFaction(Other, CurrentAttackerFaction) && NumAttackers > 0) --NumAttackers;

	if (APlayerState* PS = ResolvePS(Other))
	{
		PresentPlayers.Remove(PS);
	}
	StopCaptureTimerIfIdle();
}

// --- Server control ---
void UZoneControlComponent::ServerSetOwner(FGameplayTag NewOwner)
{
	CurrentOwnerFaction = NewOwner;
	ControlState = EZoneControlState::Controlled;
	CaptureProgress01 = 1.f;
	bInCombat = false;

	OnRep_OwnerFaction();
	OnRep_ControlState();
	OnRep_CaptureProgress();
	OnRep_Combat();
}

void UZoneControlComponent::ServerSetCombat(bool bCombat)
{
	bInCombat = bCombat;
	OnRep_Combat();

	for (TWeakObjectPtr<APlayerState> PSW : PresentPlayers)
	{
		if (APlayerState* PS = PSW.Get())
		{
			if (PS->GetClass()->ImplementsInterface(UEquipmentSystemInterface::StaticClass()))
			{
				if (UDynamicToolbarComponent* Toolbar = IEquipmentSystemInterface::Execute_ES_GetToolbar(PS))
				{
					Toolbar->TrySetCombatState(bInCombat);
				}
				if (UWieldComponent* Wield = IEquipmentSystemInterface::Execute_ES_GetWield(PS))
				{
					Wield->TrySetCombatState(bInCombat);
				}
			}
		}
	}
}

void UZoneControlComponent::ServerReset()
{
	ControlState = EZoneControlState::Neutral;
	CurrentOwnerFaction = InitialOwnerFaction;
	CaptureProgress01 = 0.f;
	bInCombat = false;
	OnRep_ControlState();
	OnRep_OwnerFaction();
	OnRep_CaptureProgress();
	OnRep_Combat();
	PushModeToSmartZone();
}

bool UZoneControlComponent::Server_ForceLiberateToPlayer_Validate(){ return true; }
void UZoneControlComponent::Server_ForceLiberateToPlayer_Implementation(){ ServerSetOwner(PlayerFaction); PushModeToSmartZone(); }

bool UZoneControlComponent::Server_ForceOwnerFaction_Validate(FGameplayTag){ return true; }
void UZoneControlComponent::Server_ForceOwnerFaction_Implementation(FGameplayTag NewOwner){ ServerSetOwner(NewOwner.IsValid()?NewOwner:InitialOwnerFaction); PushModeToSmartZone(); }

bool UZoneControlComponent::Server_ForceCombat_Validate(bool){ return true; }
void UZoneControlComponent::Server_ForceCombat_Implementation(bool bCombat){ ServerSetCombat(bCombat); }

bool UZoneControlComponent::Server_ResetZone_Validate(){ return true; }
void UZoneControlComponent::Server_ResetZone_Implementation(){ ServerReset(); }

// --- Capture tick ---
void UZoneControlComponent::StartCaptureTimer()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(CaptureTimer))
		{
			GetWorld()->GetTimerManager().SetTimer(CaptureTimer, this, &UZoneControlComponent::TickCapture, CaptureTickInterval, true);
		}
	}
}

void UZoneControlComponent::StopCaptureTimerIfIdle()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (NumAttackers <= 0 && NumDefenders <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CaptureTimer);
	}
}

void UZoneControlComponent::TickCapture()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	const float Dt = CaptureTickInterval;
	float Delta = 0.f;

	if (NumAttackers > 0)
	{
		const float AttackStrength = NumAttackers;
		const float DefenseStrength = NumDefenders * DefenderWeight;
		Delta = (AttackStrength - DefenseStrength) * CaptureRatePerAttacker * Dt;
	}
	else if (NumDefenders > 0)
	{
		Delta = -DecayRatePerSecond * Dt;
	}

	const float Old = CaptureProgress01;
	CaptureProgress01 = FMath::Clamp(Old + Delta, 0.f, 1.f);

	if (CaptureProgress01 > 0.f && CaptureProgress01 < 1.f)
	{
		if (ControlState != EZoneControlState::Contested)
		{
			ControlState = EZoneControlState::Contested;
			ServerSetCombat(true);
		}
	}
	else if (CaptureProgress01 >= 1.f)
	{
		CurrentOwnerFaction = CurrentAttackerFaction.IsValid() ? CurrentAttackerFaction : PlayerFaction;
		ControlState = EZoneControlState::Controlled;
		ServerSetCombat(false);
		PushModeToSmartZone();
	}
	else
	{
		ControlState = EZoneControlState::Controlled;
		ServerSetCombat(false);
		PushModeToSmartZone();
	}

	if (!FMath::IsNearlyEqual(Old, CaptureProgress01))
	{
		OnCaptureProgress.Broadcast(CaptureProgress01, CurrentAttackerFaction);
		OnRep_CaptureProgress();
	}
	OnRep_ControlState();
}

void UZoneControlComponent::PushModeToSmartZone()
{
	if (!SmartZone.IsValid()) return;

	if (ControlState == EZoneControlState::Controlled && CurrentOwnerFaction == PlayerFaction)
	{
		SmartZone->SetZoneTags(OutpostZoneTags.Num() > 0 ? OutpostZoneTags : SafeZoneTags);
	}
	else
	{
		SmartZone->SetZoneTags(bInCombat ? CombatZoneTags : SafeZoneTags);
	}
}
