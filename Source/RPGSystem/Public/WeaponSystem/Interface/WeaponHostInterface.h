// WeaponHostInterface.h
#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponHostInterface.generated.h"

class USkeletalMeshComponent;
class UAbilitySystemComponent;
class AController;

UINTERFACE(BlueprintType)
class UWeaponHostInterface : public UInterface
{
	GENERATED_BODY()
};

class IWeaponHostInterface
{
	GENERATED_BODY()
public:
	/** Character's mesh to attach weapons to (hands/back/etc). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon|Host")
	USkeletalMeshComponent* GetWeaponAttachMesh() const;

	/** Ability System (optional; return null if not using GAS yet). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon|Host")
	UAbilitySystemComponent* GetAbilitySystem() const;

	/** Owning Controller (helps with replication ownership). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Weapon|Host")
	AController* GetOwningController() const;
};
