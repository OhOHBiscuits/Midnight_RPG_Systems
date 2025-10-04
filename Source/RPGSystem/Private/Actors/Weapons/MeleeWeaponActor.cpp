#include "Actors/Weapons/MeleeWeaponActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

AMeleeWeaponActor::AMeleeWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMeleeWeaponActor::BeginPlay()
{
	Super::BeginPlay();
}

void AMeleeWeaponActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAttackWindowOpen)
	{
		DoTraceAndHit();
	}
}

void AMeleeWeaponActor::SetAttackWindow(bool bOpen)
{
	bAttackWindowOpen = bOpen;
	if (!bOpen)
	{
		LastSocketPositions.Reset();
	}
}

UMeleeWeaponItemDataAsset* AMeleeWeaponActor::GetMeleeData() const
{
	return Cast<UMeleeWeaponItemDataAsset>(GetItemData());
}

void AMeleeWeaponActor::DoTraceAndHit()
{
	// Minimal placeholder; hook your actual hit logic/GE application here
	// Kept empty to avoid assumptions about sockets and collision channels
}
