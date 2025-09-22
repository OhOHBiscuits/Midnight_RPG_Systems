// WeaponItemDataAsset_Melee.h
#pragma once
#include "CoreMinimal.h"
#include "WeaponItemDataAsset.h"
#include "MeleeWeaponItemDataAsset.generated.h"

UCLASS(BlueprintType)
class RPGSYSTEM_API UMeleeWeaponItemDataAsset : public UWeaponItemDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee", meta=(ClampMin="0.0"))
	float BaseDamage = 10.f;

	/** Blade/edge sockets for traces (e.g., Start, Mid, Tip). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
	TArray<FName> TraceSockets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee", meta=(ClampMin="0.0"))
	float TraceRadius = 5.f;

	/** If true, use this asset's WorldMesh sockets for traces when equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Melee")
	bool bUseWorldMeshForTrace = true;
};
