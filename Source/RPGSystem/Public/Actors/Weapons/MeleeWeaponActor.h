// MeleeWeaponActor.h
#pragma once
#include "CoreMinimal.h"
#include "BaseWeaponActor.h"
#include "Inventory/MeleeWeaponItemDataAsset.h"
#include "MeleeWeaponActor.generated.h"

UCLASS(Blueprintable)
class RPGSYSTEM_API AMeleeWeaponActor : public ABaseWeaponActor
{
	GENERATED_BODY()
public:
	AMeleeWeaponActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|State")
	bool bAttackWindowOpen = false;

	// last trace points for continuous sweep
	UPROPERTY(Transient)
	TArray<FVector> LastSocketPositions;

public:
	/** Open/close the server-authoritative damage window (call from montage notify or ability). */
	UFUNCTION(BlueprintCallable, Category="Melee|Attack")
	void SetAttackWindow(bool bOpen);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	UMeleeWeaponItemDataAsset* GetMeleeData() const;
	void DoTraceAndHit();
};
