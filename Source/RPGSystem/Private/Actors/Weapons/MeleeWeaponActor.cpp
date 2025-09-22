// MeleeWeaponActor.cpp
#include "Actors/Weapons/MeleeWeaponActor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"

AMeleeWeaponActor::AMeleeWeaponActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMeleeWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	LastSocketPositions.Reset();
}

void AMeleeWeaponActor::SetAttackWindow(bool bOpen)
{
	if (!HasAuthority()) return;
	bAttackWindowOpen = bOpen;
	LastSocketPositions.Reset();
}

void AMeleeWeaponActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bAttackWindowOpen)
	{
		DoTraceAndHit();
	}
}

UMeleeWeaponItemDataAsset* AMeleeWeaponActor::GetMeleeData() const
{
	return Cast<UMeleeWeaponItemDataAsset>(ItemData.Get());
}

void AMeleeWeaponActor::DoTraceAndHit()
{
	UMeleeWeaponItemDataAsset* D = GetMeleeData();
	if (!D || D->TraceSockets.Num() < 2) return;

	// Choose which mesh to read sockets from (in-hand = skeletal, else static)
	const bool bUseSkel = bIsEquipped && !bHolstered && EquippedSkeletalComp && EquippedSkeletalComp->GetSkeletalMeshAsset();
	const USceneComponent* RefComp = bUseSkel ? (USceneComponent*)EquippedSkeletalComp : (USceneComponent*)Mesh;

	TArray<FVector> Curr;
	for (FName S : D->TraceSockets)
	{
		Curr.Add(RefComp->GetSocketLocation(S));
	}

	// if we have previous, sweep between them
	if (LastSocketPositions.Num() == Curr.Num())
	{
		for (int32 i=1; i<Curr.Num(); ++i)
		{
			const FVector A0 = LastSocketPositions[i-1];
			const FVector A1 = Curr[i-1];
			const FVector B0 = LastSocketPositions[i];
			const FVector B1 = Curr[i];

			// simple mid-segment sweep
			const FVector Start = (A1 + B1) * 0.5f;
			const FVector End   = (A0 + B0) * 0.5f;

			TArray<FHitResult> Hits;
			UKismetSystemLibrary::SphereTraceMulti(
				this, Start, End, D->TraceRadius,
				UEngineTypes::ConvertToTraceType(ECC_Pawn),
				false, {this, GetOwner()}, EDrawDebugTrace::None, Hits, true);

			for (const FHitResult& H : Hits)
			{
				AActor* Other = H.GetActor();
				if (Other && Other != GetOwner())
				{
					// TODO: Apply damage/effects here (GAS or custom). For now just one hit per tick per actor.
					// You could maintain a "HitActorsThisSwing" set to avoid double-processing.
				}
			}
		}
	}

	LastSocketPositions = Curr;
}
