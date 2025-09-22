#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "Inventory/ItemDataAsset.h"
#include "WeaponItemDataAsset.generated.h"

class UAnimMontage;
class UGameplayEffect;

/** Shared mod socket info for all weapons */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FWeaponModSocket
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mod")
	FName Socket = NAME_None;

	/** Category this socket accepts (e.g., Mod.Muzzle, Mod.Scope) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mod")
	FGameplayTag AllowedCategory;
};

/** Base for both melee & ranged weapons */
UCLASS(BlueprintType)
class RPGSYSTEM_API UWeaponItemDataAsset : public UItemDataAsset
{
	GENERATED_BODY()
public:
	// Sockets on character/mesh where weapon attaches
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attach")
	FName EquippedSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Attach")
	FName HolsteredSocket = NAME_None;

	// Holster/unholster animations
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Anim")
	TSoftObjectPtr<UAnimMontage> HolsterMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Anim")
	TSoftObjectPtr<UAnimMontage> UnholsterMontage;

	// Skill + XP on successful hit/contact
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Progression")
	FGameplayTag WeaponSkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Progression", meta=(ClampMin="0"))
	int32 XPPerHit = 0;

	// Damage effects this weapon applies when it hits
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
	bool bUseDamageEffects = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage", meta=(EditCondition="bUseDamageEffects"))
	TArray<FGameplayTag> DamageEffectTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage", meta=(EditCondition="bUseDamageEffects"))
	TArray<TSoftClassPtr<UGameplayEffect>> DamageEffects;

	/** Switch movement set while this weapon is equipped (drive anim sets via tag). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Movement")
	FGameplayTag MovementSetTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Meshes")
	bool bHasSkeletalVariant = false;

	/** Skeletal version of the weapon (for slide/bolt/hammer animations, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Meshes", meta=(EditCondition="bHasSkeletalVariant"))
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Optional AnimInstance class for the skeletal variant. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Meshes", meta=(EditCondition="bHasSkeletalVariant"))
	TSubclassOf<UAnimInstance> SkeletalAnimClass;

	/** Prefer skeletal mesh while EQUIPPED (default true if you have moving parts). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Meshes", meta=(EditCondition="bHasSkeletalVariant"))
	bool bUseSkeletalWhenEquipped = true;

	/** Prefer skeletal mesh while DROPPED/HOLSTERED (default false for perf). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Meshes", meta=(EditCondition="bHasSkeletalVariant"))
	bool bUseSkeletalWhenDropped = false;
};
