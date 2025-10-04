// RangedWeaponActor.cpp
#include "Actors/Weapons/RangedWeaponActor.h"
#include "Inventory/RangedWeaponItemDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"

ARangedWeaponActor::ARangedWeaponActor()
{
	bReplicates = true;
}
void ARangedWeaponActor::BeginPlay()
{
	Super::BeginPlay();

	if (URangedWeaponItemDataAsset* D = GetRangedData())
	{
		if (HasAuthority())
		{
			CurrentMagazine = FMath::Max(0, D->MagazineSize);
			bHasChamber     = false;
			ChambersLoaded  = FMath::Clamp(ChambersLoaded, 0, D->NumChambers);
		}
	}
}

void ARangedWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARangedWeaponActor, CurrentMagazine);
	DOREPLIFETIME(ARangedWeaponActor, bHasChamber);
	DOREPLIFETIME(ARangedWeaponActor, ChambersLoaded);
}

URangedWeaponItemDataAsset* ARangedWeaponActor::GetRangedData() const
{
	// Uses the tiny accessor we added to ABaseWorldItemActor
	return Cast<URangedWeaponItemDataAsset>(GetItemData());
}


bool ARangedWeaponActor::HasAmmoToFire() const
{
	if (const URangedWeaponItemDataAsset* D = GetRangedData())
	{
		if (D->NumChambers > 1)
		{
			return ChambersLoaded > 0;
		}
		// single-chamber model
		if (D->bHasChamber)
		{
			return bHasChamber || CurrentMagazine > 0;
		}
		return CurrentMagazine > 0;
	}
	return false;
}

void ARangedWeaponActor::ConsumeAfterFire()
{
	if (URangedWeaponItemDataAsset* D = GetRangedData())
	{
		if (D->NumChambers > 1)
		{
			ChambersLoaded = FMath::Max(0, ChambersLoaded - 1);
			return;
		}

		if (D->bHasChamber)
		{
			if (bHasChamber)
			{
				// fired chambered round; auto-feed next if mag has ammo
				bHasChamber = false;
				if (CurrentMagazine > 0)
				{
					--CurrentMagazine;
					bHasChamber = true;
				}
			}
			else if (CurrentMagazine > 0)
			{
				// fired using immediate feed (edge case)
				--CurrentMagazine;
			}
		}
		else
		{
			if (CurrentMagazine > 0) --CurrentMagazine;
		}
	}
}

void ARangedWeaponActor::TopOffReload()
{
	if (URangedWeaponItemDataAsset* D = GetRangedData())
	{
		if (D->NumChambers > 1)
		{
			// per-chamber reload (e.g., DB shotgun)
			ChambersLoaded = D->NumChambers; // assume infinite reserve for sample; hook your ammo pool here
			return;
		}

		CurrentMagazine = D->MagazineSize;
		if (D->bHasChamber && !bHasChamber && CurrentMagazine > 0)
		{
			--CurrentMagazine;
			bHasChamber = true;
		}
	}
}

void ARangedWeaponActor::FirePressed()
{
	if (!HasAuthority()) { Server_Fire(); return; }
	Server_Fire();
}

void ARangedWeaponActor::Server_Fire_Implementation()
{
	if (!HasAuthority()) return;
	if (!bIsEquipped || bHolstered) return;
	if (!HasAmmoToFire()) return;

	// TODO: spawn projectile/hitscan, apply recoil/spread, play cues, apply GAS effects
	ConsumeAfterFire();
}

void ARangedWeaponActor::ReloadPressed()
{
	if (!HasAuthority()) { Server_Reload(); return; }
	Server_Reload();
}

void ARangedWeaponActor::Server_Reload_Implementation()
{
	if (!HasAuthority()) return;
	TopOffReload();
}
void ARangedWeaponActor::TriggerFireAV(const FName OverrideMuzzleSocket)
{
	// You can call this from server code where the shot is validated/applied.
	// If you sometimes call it on clients (prediction), the multicast still
	// ensures everyone hears/sees it.
	Multicast_PlayFireAV(OverrideMuzzleSocket);
}

void ARangedWeaponActor::Multicast_PlayFireAV_Implementation(const FName MuzzleSocket)
{
	// 1) Find where to attach AV (skeletal preferred, else static, else root)
	FName SocketToUse = MuzzleSocket;
	USceneComponent* AttachComp = GetAVAttachComponent(SocketToUse);
	const FTransform SpawnXform = AttachComp
		? AttachComp->GetSocketTransform(SocketToUse, RTS_World)
		: GetActorTransform();

	// 2) Spawn muzzle FX (you already had this logic – keep whatever you use)
	//    Example (pseudo):
	// SpawnMuzzleFX(AttachComp, SocketToUse);

	// 3) Play fire sound from data asset (if set)
	const URangedWeaponItemDataAsset* Data = Cast<URangedWeaponItemDataAsset>(GetItemData());
	if (Data)
	{
		if (USoundBase* Snd = Data->GetFireSoundSync())
		{
			if (AttachComp)
			{
				UGameplayStatics::SpawnSoundAttached(
					Snd, AttachComp, SocketToUse,
					FVector::ZeroVector, EAttachLocation::SnapToTarget, true // bStopWhenAttachedToDestroyed
				);
			}
			else
			{
				UGameplayStatics::PlaySoundAtLocation(this, Snd, SpawnXform.GetLocation());
			}
		}
	}
}

USceneComponent* ARangedWeaponActor::GetAVAttachComponent(FName& OutSocket) const
{
	// Prefer skeletal mesh if present
	if (USkeletalMeshComponent* SK = FindComponentByClass<USkeletalMeshComponent>())
	{
		// If no override provided, you probably have a socket on the weapon data
		if (OutSocket.IsNone())
		{
			if (const URangedWeaponItemDataAsset* Data = Cast<URangedWeaponItemDataAsset>(GetItemData()))
			{
				OutSocket = Data->MuzzleSocketName; // assuming you added this earlier
			}
		}
		return SK;
	}

	// Fallback to static mesh
	if (UStaticMeshComponent* SM = FindComponentByClass<UStaticMeshComponent>())
	{
		// StaticMesh sockets work the same way if you’ve authored them
		if (OutSocket.IsNone())
		{
			if (const URangedWeaponItemDataAsset* Data = Cast<URangedWeaponItemDataAsset>(GetItemData()))
			{
				OutSocket = Data->MuzzleSocketName;
			}
		}
		return SM;
	}

	// Final fallback: actor root (no socket)
	OutSocket = NAME_None;
	return GetRootComponent();
}