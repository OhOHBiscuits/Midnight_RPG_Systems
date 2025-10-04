// AModularRangedWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "Actors/Weapons/RangedWeaponActor.h"
#include "GameplayTagContainer.h"
#include "ModularRangedWeapon.generated.h"

class UStaticMeshComponent;

/** One installed mod entry replicated to clients. */
USTRUCT(BlueprintType)
struct FInstalledWeaponMod
{
	GENERATED_BODY()

	/** Socket name on the weapon (must appear in URangedWeaponItemDataAsset::ModSockets). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Socket = NAME_None;

	/** ItemID tag of the mod (resolved via InventoryAssetManager/InventoryHelpers). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTag ModItemId;

	/** Category of the mod (e.g., Mod.Muzzle, Mod.Scope). Used for validation against socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTag Category;

	FInstalledWeaponMod() {}
	FInstalledWeaponMod(FName InSocket, FGameplayTag InItemId, FGameplayTag InCategory)
		: Socket(InSocket), ModItemId(InItemId), Category(InCategory) {}
};

/**
 * Modular ranged weapon that supports socketed attachments.
 * - Reads allowed sockets from URangedWeaponItemDataAsset::ModSockets.
 * - Each installed mod is an item (UItemDataAsset) resolved by tag; its WorldMesh is attached as the visual.
 * - Server-authoritative; InstalledMods replicate and rebuild visuals on clients.
 */
UCLASS(Blueprintable)
class RPGSYSTEM_API AModularRangedWeapon : public ARangedWeaponActor
{
	GENERATED_BODY()
public:
	AModularRangedWeapon();

protected:
	/** Replicated list of installed mods (one per socket). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_InstalledMods, Category="Mods")
	TArray<FInstalledWeaponMod> InstalledMods;

	/** Runtime components for visual meshes keyed by socket. Not replicated; rebuilt from InstalledMods. */
	UPROPERTY(Transient)
	TMap<FName, UStaticMeshComponent*> RuntimeModMeshes;

public:
	/** Install (or replace) a mod on a socket. Call on any machine; routes to server. */
	UFUNCTION(BlueprintCallable, Category="Mods")
	void InstallModOnSocket(FName Socket, FGameplayTag ModItemId, FGameplayTag Category);

	/** Remove a mod from a socket. Call on any machine; routes to server. */
	UFUNCTION(BlueprintCallable, Category="Mods")
	void RemoveModFromSocket(FName Socket);

	/** Returns a copy of the current installed mods. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mods")
	TArray<FInstalledWeaponMod> GetInstalledMods() const { return InstalledMods; }

	/** Check if a category is allowed on a given socket (reads weapon data asset). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mods")
	bool CanInstallOnSocket(FName Socket, FGameplayTag Category) const;

protected:
	// RPCs
	UFUNCTION(Server, Reliable)
	void Server_InstallModOnSocket(FName Socket, FGameplayTag ModItemId, FGameplayTag Category);

	UFUNCTION(Server, Reliable)
	void Server_RemoveModFromSocket(FName Socket);

	// Rebuild visuals when InstalledMods changes
	UFUNCTION()
	void OnRep_InstalledMods();

	// Helpers
	void RebuildAllModVisuals();          // destroy & recreate components for each InstalledMods entry
	void ApplyModVisual(const FInstalledWeaponMod& Mod); // attach a single mod component
	int32 FindInstalledIndexBySocket(FName Socket) const;

	// Overridden to refresh mod visuals when base visuals change (e.g., swap to skeletal/static)
	virtual void ApplyItemDataVisuals() override;

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
