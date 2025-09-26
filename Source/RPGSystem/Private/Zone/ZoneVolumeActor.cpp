#include "Zone/ZoneVolumeActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

AZoneVolumeActor::AZoneVolumeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	RootComponent = ZoneBounds;

	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneBounds->SetGenerateOverlapEvents(true);
	ZoneBounds->SetBoxExtent(FVector(200.f));
	ZoneBounds->bHiddenInGame = true;

	Zone    = CreateDefaultSubobject<USmartZoneComponent>(TEXT("SmartZone"));
	Control = CreateDefaultSubobject<UZoneControlComponent>(TEXT("ZoneControl"));
}

void AZoneVolumeActor::BeginPlay()
{
	Super::BeginPlay();

	ZoneBounds->OnComponentBeginOverlap.AddDynamic(this, &AZoneVolumeActor::OnZoneBegin);
	ZoneBounds->OnComponentEndOverlap  .AddDynamic(this, &AZoneVolumeActor::OnZoneEnd);

	if (Zone && DefaultZoneTags.Num() > 0)
	{
		Zone->SetZoneTags(DefaultZoneTags);
	}

	if (Zone)
	{
		Zone->OnZoneEntered.AddDynamic(this, &AZoneVolumeActor::ForwardZoneEntered);
		Zone->OnZoneExited .AddDynamic(this, &AZoneVolumeActor::ForwardZoneExited);
	}
	if (Control)
	{
		Control->OnControlChanged .AddDynamic(this, &AZoneVolumeActor::ForwardControlChanged);
		Control->OnCaptureProgress.AddDynamic(this, &AZoneVolumeActor::ForwardCaptureProgress);
		Control->OnCombatChanged  .AddDynamic(this, &AZoneVolumeActor::ForwardCombatChanged);
	}
}

bool AZoneVolumeActor::IsPlayerActor(AActor* Other)
{
	if (!Other) return false;
	if (const APawn* P = Cast<APawn>(Other))
	{
		return (P->GetPlayerState() != nullptr);
	}
	return false;
}

void AZoneVolumeActor::OnZoneBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (bOnlyAffectPlayers && !IsPlayerActor(Other)) return;
	if (Zone)    Zone->HandleActorBeginOverlap(Other);
	if (Control) Control->NotifyActorEntered(Other);
}

void AZoneVolumeActor::OnZoneEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (bOnlyAffectPlayers && !IsPlayerActor(Other)) return;
	if (Zone)    Zone->HandleActorEndOverlap(Other);
	if (Control) Control->NotifyActorLeft(Other);
}

void AZoneVolumeActor::ForwardZoneEntered(AActor* Target, const FGameplayTagContainer& ZoneTags)
{
	OnZoneEntered.Broadcast(Target, ZoneTags);
}
void AZoneVolumeActor::ForwardZoneExited(AActor* Target, const FGameplayTagContainer& ZoneTags)
{
	OnZoneExited.Broadcast(Target, ZoneTags);
}
void AZoneVolumeActor::ForwardControlChanged(EZoneControlState NewState, FGameplayTag ControlledByFaction)
{
	OnControlChanged.Broadcast(NewState, ControlledByFaction);
}
void AZoneVolumeActor::ForwardCaptureProgress(float Progress01, FGameplayTag AttackerFaction)
{
	OnCaptureProgress.Broadcast(Progress01, AttackerFaction);
}
void AZoneVolumeActor::ForwardCombatChanged(bool bInCombat)
{
	OnCombatChanged.Broadcast(bInCombat);
}

void AZoneVolumeActor::SetBoxExtent(FVector NewExtent, bool bUpdateOverlaps)
{
	if (ZoneBounds) ZoneBounds->SetBoxExtent(NewExtent, bUpdateOverlaps);
}

void AZoneVolumeActor::SetZoneTags(const FGameplayTagContainer& NewZoneTags)
{
	if (Zone) Zone->SetZoneTags(NewZoneTags);
}

const FGameplayTagContainer& AZoneVolumeActor::GetZoneTags() const
{
	static FGameplayTagContainer Empty;
	return Zone ? Zone->GetZoneTags() : Empty;
}
