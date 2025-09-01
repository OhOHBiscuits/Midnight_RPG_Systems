#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Crafting/CraftingTypes.h"  
#include "GA_Craft_Generic.generated.h"

class UCraftingRecipeDataAsset;
class UCraftingStationComponent;

UCLASS()
class RPGSYSTEM_API UGA_Craft_Generic : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Craft_Generic();

protected:
	// Native override (use TriggerEventData to read OptionalObject / OptionalObject2)
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility,
							bool bWasCancelled) override;

private:
	TWeakObjectPtr<UCraftingStationComponent> BoundStation;

	bool DoCraft_Internal(const UCraftingRecipeDataAsset* Recipe, AActor* StationActor);

	UFUNCTION()
	void OnStationCraftStarted(const struct FCraftingRequest& Request, float FinalTime, const struct FSkillCheckResult& Check);

	UFUNCTION()
	void OnStationCraftCompleted(const struct FCraftingRequest& Request, const struct FSkillCheckResult& Check, bool bSuccess);

	void ClearBindings();
};
