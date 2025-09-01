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
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnStationCraftStarted(const FCraftingJob& Job);

	UFUNCTION()
	void OnStationCraftFinished(const FCraftingJob& Job, bool bSuccess);

private:
	UPROPERTY()
	TObjectPtr<UCraftingStationComponent> BoundStation = nullptr;
};
