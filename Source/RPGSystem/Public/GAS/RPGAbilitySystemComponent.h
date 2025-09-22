// RPGAbilitySystemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "RPGAbilitySystemComponent.generated.h"

/**
 * Thin ASC wrapper with convenience helpers.
 * GAS-only (no custom stat bridges).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	URPGAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* ------------------------------------------------------------
	 *  Quick install helpers (so your old calls compile again)
	 * ------------------------------------------------------------ */

	/** Create or return an existing URPGAbilitySystemComponent on the Actor. */
	static URPGAbilitySystemComponent* AddTo(AActor* Owner, FName ComponentName = TEXT("AbilitySystem"));

	/** Initialize Ability Actor Info (Owner + Avatar). Safe to call multiple times. */
	UFUNCTION(BlueprintCallable, Category="GAS|Setup")
	void InitializeForActor(AActor* InOwnerActor, AActor* InAvatarActor);

	/* ------------------------------------------------------------
	 *  Quality-of-life accessors
	 * ------------------------------------------------------------ */
	UFUNCTION(BlueprintPure, Category="GAS")
	AActor* GetASCOwner()  const { return GetOwnerActor(); }

	UFUNCTION(BlueprintPure, Category="GAS")
	AActor* GetASCAvatar() const { return GetAvatarActor(); }

	/** Build a context and optionally attach a SourceObject. */
	UFUNCTION(BlueprintCallable, Category="GAS")
	FGameplayEffectContextHandle MakeEffectContextWithSourceObject(UObject* InSourceObject);

	/**
	 * Apply a GameplayEffect (that expects a SetByCaller float) to *this* ASC (self target).
	 */
	UFUNCTION(BlueprintCallable, Category="GAS")
	FActiveGameplayEffectHandle ApplyGEWithSetByCallerToSelf(
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag                 SetByCallerTag,
		float                        Magnitude,
		float                        EffectLevel = 1.f,
		UObject*                     SourceObject = nullptr,
		int32                        Stacks = 1);

	/**
	 * Apply a GameplayEffect (that expects a SetByCaller float) to another actor's ASC.
	 * Uses Instigator/EffectCauser if provided, otherwise uses our owner/avatar.
	 */
	UFUNCTION(BlueprintCallable, Category="GAS")
	static FActiveGameplayEffectHandle ApplyGEWithSetByCallerToTarget(
		AActor*                      TargetActor,
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag                 SetByCallerTag,
		float                        Magnitude,
		float                        EffectLevel = 1.f,
		UObject*                     SourceObject = nullptr,
		int32                        Stacks = 1,
		AActor*                      Instigator = nullptr,
		AActor*                      EffectCauser = nullptr);
};
