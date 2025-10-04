#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemDataAsset.h"
#include "WeaponModItemDataAsset.generated.h"

/** Which stat this modifier touches (focused on ranged for now). */
UENUM(BlueprintType)
enum class ERangedWeaponStat : uint8
{
	MagazineCapacity UMETA(DisplayName="Magazine Capacity"),
	FireRateRPM      UMETA(DisplayName="Fire Rate (RPM)"),
	AccuracyDeg      UMETA(DisplayName="Accuracy (deg)"),
	SpreadDeg        UMETA(DisplayName="Spread (deg)"),
	RecoilKick       UMETA(DisplayName="Recoil Kick"),
	ReloadSeconds    UMETA(DisplayName="Reload (s)"),
	Handling         UMETA(DisplayName="Handling"),
	EffectiveRange   UMETA(DisplayName="Effective Range (cm)"),
	MuzzleVelocity   UMETA(DisplayName="Muzzle Velocity (cm/s)"),
	BulletDropScale  UMETA(DisplayName="Bullet Drop Scale"),
	Loudness         UMETA(DisplayName="Loudness"),
};

UENUM(BlueprintType)
enum class EStatModOp : uint8
{
	Set UMETA(DisplayName="Set (override)"),
	Add UMETA(DisplayName="Add"),
	Mul UMETA(DisplayName="Multiply"),
};

/** One stat operation (e.g., Set MagazineCapacity = 100). */
USTRUCT(BlueprintType)
struct FRangedWeaponStatMod
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stat")
	ERangedWeaponStat Stat = ERangedWeaponStat::MagazineCapacity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stat")
	EStatModOp Op = EStatModOp::Set;

	/** Value to apply (meaning depends on Op). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stat")
	float Value = 0.f;
};

/**
 * A weapon mod (magazine, scope, etc.). Child of your UItemDataAsset
 * so it fits naturally into your inventory/attachment flow.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UWeaponModItemDataAsset : public UItemDataAsset
{
	GENERATED_BODY()
public:
	/** Category this mod belongs to (e.g., Mod.Magazine, Mod.Scope). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mod")
	FGameplayTag ModCategory;

	/** Stat edits this mod applies when equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mod")
	TArray<FRangedWeaponStatMod> RangedStatMods;
};
