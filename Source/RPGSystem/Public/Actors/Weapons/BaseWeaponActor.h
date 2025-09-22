#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemDataAsset.h" 
#include "Actors/BaseWorldItemActor.h"
#include "BaseWeaponActor.generated.h"

class UWeaponItemDataAsset;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EWeaponVisualState : uint8
{
	Dropped,     // world/dropped or holstered on body
	Holstered,   // (optional distinct state if you want)
	Equipped     // in-hands, first/third person
};
UCLASS(Abstract)
class RPGSYSTEM_API ABaseWeaponActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	ABaseWeaponActor();

	/** When true, in-hand display uses SkeletalMesh; world/holstered may use static */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Visual")
	bool bUseSkeletalWhenEquipped = true;

	/** Equipped skeletal component (only created/visible when needed) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Visual")
	USkeletalMeshComponent* EquippedSkeletalComp = nullptr;

	/** Weapon state replicated */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon|State")
	bool bIsEquipped = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon|State")
	bool bHolstered = false;

	UFUNCTION(BlueprintCallable, Category="Weapon|Visual")
	void SetVisualState(EWeaponVisualState NewState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Sockets")
	bool GetWeaponSocketTransform(FName SocketName, FTransform& OutWorldXform) const;

protected:
	
	UPROPERTY(Transient)
	UWeaponItemDataAsset* CachedWeaponData = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Now virtual in base; safe to override here */
	virtual void ApplyItemDataVisuals() override;

	/** Attach/detach helpers for in-hand vs world */
	virtual void RefreshAttachment();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Components")
	USkeletalMeshComponent* SkelMesh;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WorldItem|Data")
	UItemDataAsset* GetItemData() const { return ItemData.Get(); }

	// Replicated so clients show the same thing
	UPROPERTY(ReplicatedUsing=OnRep_VisualState, BlueprintReadOnly, Category="Weapon|Visual")
	EWeaponVisualState VisualState = EWeaponVisualState::Dropped;

	UFUNCTION()
	void OnRep_VisualState();	
	void UpdateVisualsForState(const UWeaponItemDataAsset* Data);

	
	
};
