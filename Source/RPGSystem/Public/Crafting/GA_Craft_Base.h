#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GSCGameplayAbility.h"          // GSC base
#include "GA_Craft_Base.generated.h"

struct FGameplayEventData;

class UCraftingRecipeDataAsset;
class UInventoryComponent;

// UI events (Blueprints can bind)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCraftEvent, FGameplayTag, RecipeID, int32, Quantity, bool, bSuccess);

/**
 * Generic Craft Ability (GSC base).
 * Activate via gameplay event payload:
 *  - OptionalObject  : UCraftingRecipeDataAsset*
 *  - OptionalObject2 : AActor* (Workstation, optional)
 *  - EventMagnitude  : Quantity (float; clamped to int >=1)
 */
UCLASS()
class RPGSYSTEM_API UGA_Craft_Base : public UGSCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Craft_Base();

	// Event that triggers crafting (e.g., Event.Craft.Start)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Ability|Config", meta=(Categories="Event"))
	FGameplayTag StartEventTag;

	// Optional global gate tag required on ASC
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Crafting-Ability|Config", meta=(Categories="Craft"))
	FGameplayTag GlobalCraftingEnableTag;

	// UI/Event dispatchers
	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Ability|Events")
	FCraftEvent OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Ability|Events")
	FCraftEvent OnCraftCompleted;

	// GA entry point (server-only ability)
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Our internal handler invoked by ActivateAbility
	void HandleActivateFromEvent(const FGameplayEventData& EventData);

	// Cached per activation
	const UCraftingRecipeDataAsset* ActiveRecipe = nullptr;
	TWeakObjectPtr<AActor>          ActiveWorkstation;
	int32                           RequestedQuantity = 1;
	TWeakObjectPtr<UInventoryComponent> CachedInventory;

	// Validation
	bool Server_ValidateAndInit(const FGameplayEventData& EventData);
	bool CheckUnlockTag() const;
	bool CheckStationGates() const;
	bool HasSufficientInputs(int32 InQuantity) const;

	// Flow
	void Server_StartCraft();
	void Server_FinishCraft();

	// Helpers
	float                ComputeFinalTimeSeconds(int32 InQuantity) const;
	UInventoryComponent* ResolveInventory() const;
	int32                CountTotalByItemID(UInventoryComponent* Inv, const FGameplayTag& ItemID) const;
	bool                 ConsumeInputs(UInventoryComponent* Inv, int32 InQuantity) const;
	bool                 GrantOutputs(UInventoryComponent* Inv, int32 InQuantity) const;

	// Cosmetic (optional)
	void TryPlayMontage(float DurationSeconds);
};
