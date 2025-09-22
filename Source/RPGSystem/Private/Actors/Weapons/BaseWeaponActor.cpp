#include "Actors/Weapons/BaseWeaponActor.h"
#include "Inventory/WeaponItemDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Inventory/ItemDataAsset.h" 
#include "Net/UnrealNetwork.h"

ABaseWeaponActor::ABaseWeaponActor()
{
	// Your ABaseWorldItemActor already creates StaticMesh "Mesh" and replicates.
	bReplicates = true;

	SkelMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelMesh"));
	SkelMesh->SetupAttachment(GetRootComponent());
	SkelMesh->SetVisibility(false);
	SkelMesh->SetComponentTickEnabled(false);
	SkelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkelMesh->SetIsReplicated(true);

	// Skeletal perf defaults (good for dropped/holstered)
	SkelMesh->bEnableUpdateRateOptimizations = true;
	SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void ABaseWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseWeaponActor, VisualState);
}

void ABaseWeaponActor::ApplyItemDataVisuals()
{
	// First let base set the world static mesh from ItemData->WorldMesh
	ABaseWorldItemActor::ApplyItemDataVisuals();

	// Load & cache weapon data if present
	CachedWeaponData = Cast<UWeaponItemDataAsset>(ItemData.Get());

	if (!CachedWeaponData || !CachedWeaponData->bHasSkeletalVariant)
	{
		// No skeletal variant => hide skel, rely on static mesh
		SkelMesh->SetVisibility(false);
		SkelMesh->SetComponentTickEnabled(false);
		return;
	}

	// Load skeletal mesh if available
	if (USkeletalMesh* SM = CachedWeaponData->SkeletalMesh.LoadSynchronous())
	{
		SkelMesh->SetSkeletalMesh(SM);
		if (CachedWeaponData->SkeletalAnimClass)
		{
			SkelMesh->SetAnimInstanceClass(CachedWeaponData->SkeletalAnimClass);
		}
	}

	// Apply according to current state (in editor this runs via OnConstruction too)
	UpdateVisualsForState(CachedWeaponData);
}

void ABaseWeaponActor::UpdateVisualsForState(const UWeaponItemDataAsset* Data)
{
	const bool bPreferSkeletal =
		(VisualState == EWeaponVisualState::Equipped && Data->bUseSkeletalWhenEquipped) ||
		(VisualState != EWeaponVisualState::Equipped && Data->bUseSkeletalWhenDropped);

	// Toggle visibility & ticking
	if (bPreferSkeletal && Data->bHasSkeletalVariant && SkelMesh->GetSkeletalMeshAsset())
	{
		SkelMesh->SetVisibility(true);
		SkelMesh->SetComponentTickEnabled(true);

		Mesh->SetVisibility(false);
		Mesh->SetComponentTickEnabled(false);
	}
	else
	{
		SkelMesh->SetVisibility(false);
		SkelMesh->SetComponentTickEnabled(false);

		Mesh->SetVisibility(true);
		Mesh->SetComponentTickEnabled(true);
	}
}

void ABaseWeaponActor::OnRep_VisualState()
{
	// Make sure data is available on clients
	if (!CachedWeaponData)
	{
		CachedWeaponData = Cast<UWeaponItemDataAsset>(ItemData.Get());
	}
	if (CachedWeaponData)
	{
		UpdateVisualsForState(CachedWeaponData);
	}
}

void ABaseWeaponActor::SetVisualState(EWeaponVisualState NewState)
{
	if (HasAuthority())
	{
		VisualState = NewState;
		OnRep_VisualState(); // update locally on server
	}
	else
	{
		// If you want to drive this from client (e.g., equip), add a Server RPC that sets VisualState.
	}

	// NOTE: actual equip/holster game logic remains wherever you trigger it.
	// This only flips visible component efficiently and consistently in MP.
}
void ABaseWeaponActor::RefreshAttachment()
{
	const bool bShowSkel = bUseSkeletalWhenEquipped && bIsEquipped && !bHolstered && EquippedSkeletalComp;
	if (!EquippedSkeletalComp) return;

	EquippedSkeletalComp->SetVisibility(bShowSkel);
	Mesh->SetVisibility(!bShowSkel);
}

bool ABaseWeaponActor::GetWeaponSocketTransform(FName SocketName, FTransform& OutWorld) const
{
	// Try skeletal first
	if (SkelMesh && SkelMesh->DoesSocketExist(SocketName))
	{
		OutWorld = SkelMesh->GetSocketTransform(SocketName, RTS_World);
		return true;
	}
	// Then static
	if (Mesh && Mesh->DoesSocketExist(SocketName))
	{
		OutWorld = Mesh->GetSocketTransform(SocketName, RTS_World);
		return true;
	}
	OutWorld = (Mesh ? Mesh->GetComponentTransform() : GetActorTransform());
	return false;
}