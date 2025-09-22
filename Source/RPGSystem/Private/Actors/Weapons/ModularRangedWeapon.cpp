// AModularRangedWeapon.cpp
#include "Actors/Weapons/ModularRangedWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/RangedWeaponItemDataAsset.h"
#include "Inventory/InventoryHelpers.h" // for FindItemDataByTag
#include "Inventory/ItemDataAsset.h"

AModularRangedWeapon::AModularRangedWeapon()
{
	bReplicates = true;
}

void AModularRangedWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AModularRangedWeapon, InstalledMods);
}

int32 AModularRangedWeapon::FindInstalledIndexBySocket(FName Socket) const
{
	for (int32 i=0; i<InstalledMods.Num(); ++i)
	{
		if (InstalledMods[i].Socket == Socket) return i;
	}
	return INDEX_NONE;
}

bool AModularRangedWeapon::CanInstallOnSocket(FName Socket, FGameplayTag Category) const
{
	const URangedWeaponItemDataAsset* R = Cast<URangedWeaponItemDataAsset>(ItemData.Get());
	if (!R) return false;

	for (const FWeaponModSocket& S : R->ModSockets)
	{
		if (S.Socket == Socket)
		{
			// If AllowedCategory is set, enforce match; if empty, allow any.
			return !S.AllowedCategory.IsValid() || S.AllowedCategory == Category;
		}
	}
	return false; // no such socket defined
}

void AModularRangedWeapon::InstallModOnSocket(FName Socket, FGameplayTag ModItemId, FGameplayTag Category)
{
	if (!HasAuthority()) { Server_InstallModOnSocket(Socket, ModItemId, Category); return; }
	Server_InstallModOnSocket(Socket, ModItemId, Category);
}

void AModularRangedWeapon::RemoveModFromSocket(FName Socket)
{
	if (!HasAuthority()) { Server_RemoveModFromSocket(Socket); return; }
	Server_RemoveModFromSocket(Socket);
}

void AModularRangedWeapon::Server_InstallModOnSocket_Implementation(FName Socket, FGameplayTag ModItemId, FGameplayTag Category)
{
	// Validate vs weapon data sockets
	if (!CanInstallOnSocket(Socket, Category)) return;

	const int32 Index = FindInstalledIndexBySocket(Socket);
	if (Index == INDEX_NONE)
	{
		InstalledMods.Add(FInstalledWeaponMod(Socket, ModItemId, Category));
	}
	else
	{
		InstalledMods[Index] = FInstalledWeaponMod(Socket, ModItemId, Category);
	}

	// Rebuild on server too so listen server sees it immediately
	RebuildAllModVisuals();
	// Clients will rebuild via OnRep_InstalledMods
}

void AModularRangedWeapon::Server_RemoveModFromSocket_Implementation(FName Socket)
{
	const int32 Index = FindInstalledIndexBySocket(Socket);
	if (Index != INDEX_NONE)
	{
		InstalledMods.RemoveAt(Index);
		RebuildAllModVisuals();
	}
}

void AModularRangedWeapon::OnRep_InstalledMods()
{
	RebuildAllModVisuals();
}

void AModularRangedWeapon::ApplyItemDataVisuals()
{
	Super::ApplyItemDataVisuals();
	// Base visuals may switch between skeletal/static; rebuild mods to ensure proper parent + sockets
	RebuildAllModVisuals();
}

void AModularRangedWeapon::RebuildAllModVisuals()
{
	// Destroy old
	for (auto& Pair : RuntimeModMeshes)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	RuntimeModMeshes.Empty();

	// Recreate each installed mod
	for (const FInstalledWeaponMod& M : InstalledMods)
	{
		ApplyModVisual(M);
	}
}

void AModularRangedWeapon::ApplyModVisual(const FInstalledWeaponMod& Mod)
{
	// Resolve the mod item data
	UItemDataAsset* ModItem = UInventoryHelpers::FindItemDataByTag(this, Mod.ModItemId);
	if (!ModItem) return;

	// Pull a mesh to use for visuals (use the mod item's WorldMesh)
	UStaticMesh* MeshAsset = ModItem->GetWorldMeshSync();
	if (!MeshAsset) return;

	// Decide which component to attach to: in-hand => skeletal, otherwise static
	USceneComponent* Parent = nullptr;
	if (bIsEquipped && !bHolstered && EquippedSkeletalComp && EquippedSkeletalComp->GetSkeletalMeshAsset())
	{
		Parent = EquippedSkeletalComp;
	}
	else
	{
		Parent = Mesh;
	}

	if (!Parent) return;

	// Create component
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this);
	Comp->SetStaticMesh(MeshAsset);
	Comp->SetupAttachment(Parent, Mod.Socket);
	Comp->RegisterComponent();
	Comp->SetIsReplicated(false); // purely cosmetic; driven by InstalledMods replication
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetGenerateOverlapEvents(false);

	RuntimeModMeshes.Add(Mod.Socket, Comp);
}
