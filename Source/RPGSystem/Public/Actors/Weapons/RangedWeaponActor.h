// RangedWeaponActor.h
#pragma once
#include "CoreMinimal.h"
#include "BaseWeaponActor.h"
#include "Sound/SoundBase.h"
#include "Inventory/RangedWeaponItemDataAsset.h"
#include "RangedWeaponActor.generated.h"

class USoundBase;
class URangedWeaponItemDataAsset;


UCLASS(Blueprintable)
class RPGSYSTEM_API ARangedWeaponActor : public ABaseWeaponActor
{
	GENERATED_BODY()
public:
	ARangedWeaponActor();

protected:
	// Ammo state (replicated)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Ranged|Ammo")
	int32 CurrentMagazine = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Ranged|Ammo")
	bool bHasChamber = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Ranged|Ammo")
	int32 ChambersLoaded = 0; // for multi-chamber (DB shotgun, etc.)	

public:
	/** Server-side fire attempt. Handles mag+chamber semantics. */
	UFUNCTION(BlueprintCallable, Category="Ranged|Fire")
	void FirePressed();

	/** Server-side reload. */
	UFUNCTION(BlueprintCallable, Category="Ranged|Reload")
	void ReloadPressed();

	UFUNCTION(BlueprintCallable, Category="Weapon|AV")
	void TriggerFireAV(const FName OverrideMuzzleSocket = NAME_None);

protected:
	UFUNCTION(Server, Reliable)
	void Server_Fire();

	UFUNCTION(Server, Reliable)
	void Server_Reload();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireAV(const FName MuzzleSocket);
	void Multicast_PlayFireAV_Implementation(const FName MuzzleSocket);

	// Helper to resolve the component and socket to attach AV to
	USceneComponent* GetAVAttachComponent(FName& OutSocket) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Data")
	URangedWeaponItemDataAsset* GetRangedData() const;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;	
	


	// helpers
	bool HasAmmoToFire() const;
	void ConsumeAfterFire(); // decrement chamber/mag based on model
	void TopOffReload();     // magazine + chamber, or per-chamber
};
