#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/WeaponModItemDataAsset.h"
#include "Inventory/RangedWeaponItemDataAsset.h"
#include "WeaponStatComputeLibrary.generated.h"

/** What we compute for ranged weapons; add more as you need. */
USTRUCT(BlueprintType)
struct FRangedComputedStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	int32 MagazineCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float FireRateRPM = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float AccuracyDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float SpreadDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float RecoilKick = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float ReloadSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Handling = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float EffectiveRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MuzzleVelocity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float BulletDropScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Loudness = 1.f;
};

UCLASS()
class RPGSYSTEM_API UWeaponStatComputeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Compute all ranged stats (base + mods). Safe to call on client/server. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapons|Compute")
	static FRangedComputedStats ComputeRangedStats(
		const URangedWeaponItemDataAsset* Base,
		const TArray<UWeaponModItemDataAsset*>& EquippedMods);

	/** Convenience: just the magazine capacity. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapons|Compute")
	static int32 GetEffectiveMagazineCapacity(
		const URangedWeaponItemDataAsset* Base,
		const TArray<UWeaponModItemDataAsset*>& EquippedMods);
};
