// Source/RPGSystem/Private/Actors/Weapons/BaseWeaponActor.cpp
// All rights Reserved Midnight Entertainment Studios LLC

#include "Actors/Weapons/BaseWeaponActor.h"
#include "Inventory/WeaponItemDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

ABaseWeaponActor::ABaseWeaponActor()
{
	bReplicates = true;

	SkelMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal"));
	SkelMesh->SetupAttachment(RootComponent);
	SkelMesh->SetVisibility(false);
	EquippedSkeletalComp = SkelMesh;
}

void ABaseWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseWeaponActor, bIsEquipped);
	DOREPLIFETIME(ABaseWeaponActor, bHolstered);
	DOREPLIFETIME(ABaseWeaponActor, VisualState);
}

void ABaseWeaponActor::OnRep_VisualState()
{
	UpdateVisualsForState(Cast<UWeaponItemDataAsset>(ItemData.Get()));
}

void ABaseWeaponActor::ApplyItemDataVisuals()
{
	Super::ApplyItemDataVisuals();
	UpdateVisualsForState(Cast<UWeaponItemDataAsset>(ItemData.Get()));
}

void ABaseWeaponActor::UpdateVisualsForState(const UWeaponItemDataAsset* Data)
{
	const bool bUseSkel =
		Data && Data->bHasSkeletalVariant &&
		((VisualState == EWeaponVisualState::Equipped && Data->bUseSkeletalWhenEquipped) ||
		 (VisualState != EWeaponVisualState::Equipped && Data->bUseSkeletalWhenDropped));

	SkelMesh->SetVisibility(bUseSkel);
}

void ABaseWeaponActor::SetVisualState(EWeaponVisualState NewState)
{
	if (VisualState == NewState) return;
	VisualState = NewState;
	OnRep_VisualState();
}

bool ABaseWeaponActor::GetWeaponSocketTransform(FName SocketName, FTransform& OutWorldXform) const
{
	if (SkelMesh && SkelMesh->DoesSocketExist(SocketName))
	{
		OutWorldXform = SkelMesh->GetSocketTransform(SocketName);
		return true;
	}
	return false;
}

void ABaseWeaponActor::RefreshAttachment()
{
	// Presentation is driven by WieldComponent + anim notifies.
}
