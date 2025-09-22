// WeaponAmmoComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"

// Adjust include path to where you put the header above
#include "Inventory/RangedWeaponItemDataAsset.h"

#include "WeaponAmmoComponent.generated.h"

/**
 * Replicated ammo state + server-authoritative firing & reload logic.
 * Designed to work with magazine-only, chamber-only, multi-chamber,
 * and the new MultiChamberMagazine systems.
 */
UCLASS(ClassGroup=(Weapons), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UWeaponAmmoComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWeaponAmmoComponent();

	// ---------------- Data ----------------
	/** Currently-equipped weapon's ranged item data (not replicated; authoritative on server). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ammo")
	URangedWeaponItemDataAsset* Base = nullptr;

	/** Replicated magazine count (for mag-fed systems). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ammo")
	int32 CurrentMagAmmo = 0;

	/** Replicated single chamber flag (for single-chamber systems). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ammo")
	bool bChamberLoaded = false;

	/** Replicated multi-chamber count (0..NumChambers). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Ammo")
	int32 ChambersLoaded = 0;

	// ---------------- Setup ----------------
	/** Initialize ammo state from weapon data (call on server when equipping). */
	UFUNCTION(BlueprintCallable, Category="Ammo")
	void InitializeFromItemData(URangedWeaponItemDataAsset* InBase);

	// ---------------- Queries ----------------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ammo")
	bool IsMagazineFed() const
	{
		return Base && (Base->IsMagazineFed());
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ammo")
	bool UsesChambers() const
	{
		return Base && (Base->UsesChambers());
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ammo")
	int32 GetCurrentMag() const { return CurrentMagAmmo; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ammo")
	bool IsChamberLoaded() const { return bChamberLoaded; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ammo")
	int32 GetChambersLoaded() const { return ChambersLoaded; }

	// ---------------- Actions (Server RPCs) ----------------
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireRound();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload(bool bTactical);

	// ---------------- Rep ----------------
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
