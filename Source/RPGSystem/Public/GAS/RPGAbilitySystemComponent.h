// RPGAbilitySystemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "RPGAbilitySystemComponent.generated.h"

/** Small helpers so GAS nodes can read/write your custom stat system. */
UCLASS(ClassGroup=(RPG), BlueprintType, meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	URPGAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	

	/** Initialize GAS with Owner/Avatar (call any time the avatar changes). */
	UFUNCTION(BlueprintCallable, Category="RPG|ASC")
	void InitializeForActor(AActor* InOwnerActor, AActor* InAvatarActor);

	/** Ensure an RPG ASC exists on Target (creates, registers, and initializes if missing). */
	UFUNCTION(BlueprintCallable, Category="RPG|ASC")
	static URPGAbilitySystemComponent* AddTo(AActor* Target);

	/** Quick check */
	UFUNCTION(BlueprintPure, Category="RPG|ASC")
	static bool HasRPGASC(const AActor* Target);
	
	///Add Above--Below gets Stats form Custom system///
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetStat(const FGameplayTag& Tag, float DefaultValue = 0.f) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetStat(const FGameplayTag& Tag, float NewValue) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddToStat(const FGameplayTag& Tag, float Delta) const;
};


