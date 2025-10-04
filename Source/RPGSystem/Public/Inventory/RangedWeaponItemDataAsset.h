// RangedWeaponItemDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/SoftObjectPtr.h"
#include "GameplayTagContainer.h"

// Make sure this include points at your base weapon item data header
// (the one that declares UWeaponItemDataAsset and optionally FWeaponModSocket).
#include "WeaponItemDataAsset.h"

#include "RangedWeaponItemDataAsset.generated.h"

// ------------------ Feed system ------------------
// APPEND-ONLY! Do not reorder existing entries in a live project.
UENUM(BlueprintType)
enum class EWeaponFeedSystem : uint8
{
	Magazine              UMETA(DisplayName="Magazine-fed"),
	SingleChamber         UMETA(DisplayName="Single Chamber (no mag)"),
	MultiChamber          UMETA(DisplayName="Multi-Chamber (no mag)"),
	MultiChamberMagazine  UMETA(DisplayName="Multi-Chamber + Magazine") // NEW
};

// Forward decls
class UAnimMontage;
class UNiagaraSystem;
class UParticleSystem;
class AActor;
/**
 * Ranged weapon data that extends your base weapon item data.
 * (Shares all the base fields such as sockets, movement set, damage effects, etc.)
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API URangedWeaponItemDataAsset : public UWeaponItemDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Sockets")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Socket to use for case ejection FX / casing spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Sockets")
	FName CaseEjectSocketName = TEXT("Eject");

	/** Optional Niagara muzzle FX (preferred if Niagara module present). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|FX")
	TSoftObjectPtr<UNiagaraSystem> MuzzleFXNiagara;

	/** Optional Niagara case-eject FX. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|FX")
	TSoftObjectPtr<UNiagaraSystem> CaseEjectFXNiagara;

	/** Optional Cascade muzzle FX (fallback if you don't use Niagara). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|FX")
	TSoftObjectPtr<UParticleSystem> MuzzleFXCascade;

	/** Optional Cascade case-eject FX. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|FX")
	TSoftObjectPtr<UParticleSystem> CaseEjectFXCascade;

	/** If true, eject (FX and/or a casing actor) when firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Casing")
	bool bEjectsCasing = true;

	/** Optional actor to spawn for a physical casing (can have physics/niagara of its own). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Casing", meta=(EditCondition="bEjectsCasing"))
	TSubclassOf<AActor> CasingActorClass;

	/** Auto-destroy time for spawned casing actors. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Casing", meta=(EditCondition="bEjectsCasing", ClampMin="0.0"))
	float CasingLifeSeconds = 6.0f;
	// ------------ AMMO ------------
	/** Ammo definition (Tag helps you look up compatible ammo items). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	FGameplayTag AmmoType;

	/** Base/default magazine size (actual actor may override at runtime). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo", meta=(ClampMin="0"))
	int32 MagazineSize = 30;

	/** If true, weapon supports a chambered round (e.g., pistols, rifles). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	bool bHasChamber = true;

	/** Feed system selection (magazine, chambers, both). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	EWeaponFeedSystem FeedSystem = EWeaponFeedSystem::Magazine;

	/** Number of chambers (2 for double-barrel, etc.). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo", meta=(ClampMin="1"))
	int32 NumChambers = 1;

	/** After each shot, auto-cycle a round from mag to chamber(s) if possible. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	bool bAutoCycleOnFire = true;

	/** Allow topping off mag while leaving chamber(s) loaded. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	bool bSupportsTacticalReload = true;

	/** When reloading, automatically fill chamber(s) from the mag if available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ammo")
	bool bAutoChamberOnReload = true;

	// ------------ BALLISTICS / HANDLING (kept minimal; add more as needed) ------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ballistics", meta=(ClampMin="0.0"))
	float MuzzleVelocity = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ballistics", meta=(ClampMin="0.0"))
	float EffectiveRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float AccuracyDeg = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float FireRateRPM = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float SpreadDeg = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float RecoilKick = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float ReloadSeconds = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Handling", meta=(ClampMin="0.0"))
	float Handling = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Ballistics", meta=(ClampMin="0.0"))
	float BulletDropScale = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Audio", meta=(ClampMin="0.0"))
	float Loudness = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ranged|Audio")
	USoundBase* GetFireSoundSync() const;

	// ------------ ANIM ------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Anim")
	TSoftObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Anim")
	TSoftObjectPtr<UAnimMontage> FireMontage;

	// ------------ IK ------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|IK")
	FName OffhandIKSocket = NAME_None;

	// ------------ Mods (optional; requires FWeaponModSocket in your base header) ------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Mods", meta=(ClampMin="0"))
	int32 MaxModSlots = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ranged|Mods")
	TArray<FWeaponModSocket> ModSockets;

	// ------------ Helpers ------------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ranged|Ammo")
	bool IsMagazineFed() const
	{
		return FeedSystem == EWeaponFeedSystem::Magazine
			|| FeedSystem == EWeaponFeedSystem::MultiChamberMagazine;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ranged|Ammo")
	bool UsesChambers() const
	{
		return FeedSystem == EWeaponFeedSystem::SingleChamber
			|| FeedSystem == EWeaponFeedSystem::MultiChamber
			|| FeedSystem == EWeaponFeedSystem::MultiChamberMagazine;
	}
	
};
