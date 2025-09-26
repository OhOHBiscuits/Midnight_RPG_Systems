#include "Zone/SmartZoneComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include "EquipmentSystem/EquipmentSystemInterface.h"
#include "EquipmentSystem/DynamicToolbarComponent.h"
#include "EquipmentSystem/WieldComponent.h"

USmartZoneComponent::USmartZoneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USmartZoneComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USmartZoneComponent, ZoneTags);
}

void USmartZoneComponent::OnRep_ZoneTags()
{
	// Optional local UI hook
}

static APlayerState* SZ_ResolvePS(AActor* Target)
{
	if (!Target) return nullptr;
	if (APawn* P = Cast<APawn>(Target)) return P->GetPlayerState();
	if (AController* C = Cast<AController>(Target)) return C->PlayerState;
	return Cast<APlayerState>(Target);
}

void USmartZoneComponent::HandleActorBeginOverlap(AActor* Other)
{
	if (!bApplyOnOverlap || !Other) return;
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) { Apply_Internal(Other); }
		else { Server_ApplyZone(Other); }
	}
}

void USmartZoneComponent::HandleActorEndOverlap(AActor* Other)
{
	if (!bApplyOnOverlap || !Other) return;
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) { Remove_Internal(Other); }
		else { Server_RemoveZone(Other); }
	}
}

void USmartZoneComponent::ApplyZoneToActor(AActor* Target)
{
	if (!Target) return;
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) Apply_Internal(Target);
		else Server_ApplyZone(Target);
	}
}

void USmartZoneComponent::RemoveZoneFromActor(AActor* Target)
{
	if (!Target) return;
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority()) Remove_Internal(Target);
		else Server_RemoveZone(Target);
	}
}

void USmartZoneComponent::Apply_Internal(AActor* Target)
{
	NotifySystems_Enter(Target);
	OnZoneEntered.Broadcast(Target, ZoneTags);
}

void USmartZoneComponent::Remove_Internal(AActor* Target)
{
	NotifySystems_Exit(Target);
	OnZoneExited.Broadcast(Target, ZoneTags);
}

void USmartZoneComponent::NotifySystems_Enter(AActor* Target)
{
	if (APlayerState* PS = SZ_ResolvePS(Target))
	{
		if (PS->GetClass()->ImplementsInterface(UEquipmentSystemInterface::StaticClass()))
		{
			if (UDynamicToolbarComponent* Toolbar = IEquipmentSystemInterface::Execute_ES_GetToolbar(PS))
			{
				Toolbar->TryNotifyZoneTagsUpdated(ZoneTags);
			}
			if (bForceWieldProfileOnEnter)
			{
				if (UWieldComponent* Wield = IEquipmentSystemInterface::Execute_ES_GetWield(PS))
				{
					Wield->TrySetActiveWieldProfile(WieldProfileOnEnter);
				}
			}
		}
	}
}

void USmartZoneComponent::NotifySystems_Exit(AActor* Target)
{
	if (APlayerState* PS = SZ_ResolvePS(Target))
	{
		if (PS->GetClass()->ImplementsInterface(UEquipmentSystemInterface::StaticClass()))
		{
			if (UDynamicToolbarComponent* Toolbar = IEquipmentSystemInterface::Execute_ES_GetToolbar(PS))
			{
				FGameplayTagContainer Empty;
				Toolbar->TryNotifyZoneTagsUpdated(Empty);
			}
			if (bRevertWieldProfileOnExit)
			{
				if (UWieldComponent* Wield = IEquipmentSystemInterface::Execute_ES_GetWield(PS))
				{
					Wield->TrySetActiveWieldProfile(WieldProfileOnExit);
				}
			}
		}
	}
}

// RPCs
bool USmartZoneComponent::Server_ApplyZone_Validate(AActor*) { return true; }
void USmartZoneComponent::Server_ApplyZone_Implementation(AActor* Target) { Apply_Internal(Target); }

bool USmartZoneComponent::Server_RemoveZone_Validate(AActor*) { return true; }
void USmartZoneComponent::Server_RemoveZone_Implementation(AActor* Target) { Remove_Internal(Target); }
