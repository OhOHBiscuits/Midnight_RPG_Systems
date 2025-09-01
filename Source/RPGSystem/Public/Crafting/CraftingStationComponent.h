#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Crafting/CraftingTypes.h"
#include "Progression/SkillCheckTypes.h"
#include "CraftingStationComponent.generated.h"

class UInventoryComponent;
class UCheckDefinition;
class UGameplayEffect;
class UCraftingRecipeDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftStarted, const FCraftingJob&, Job);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftFinished, const FCraftingJob&, Job, bool, bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingStationComponent();

	// Optional “discipline” / domain tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	FGameplayTag DomainTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	TObjectPtr<UCheckDefinition> DefaultCheck = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Setup")
	TSubclassOf<UGameplayEffect> AddXPEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DefaultXPOnStartFraction = 0.0f;

	// Recipes the station exposes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recipes")
	TArray<TObjectPtr<const UCraftingRecipeDataAsset>> AvailableRecipes;

	// Output routing knobs (needed by your Fuel workstation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
	TObjectPtr<UInventoryComponent> OutputInventoryOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
	bool bOutputToStationInventory = true;

	UFUNCTION(BlueprintCallable, Category="Crafting", meta=(DefaultToSelf="Instigator"))
	bool StartCraft(AActor* Instigator, const FCraftingRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Crafting", meta=(DefaultToSelf="Instigator"))
	bool StartCraftFromRecipe(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintCallable, Category="Crafting")
	void CancelCraft();

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCraftStarted OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCraftFinished OnCraftFinished;

	UPROPERTY(BlueprintReadOnly, Category="State")
	bool bIsCrafting = false;

	UPROPERTY(BlueprintReadOnly, Category="State")
	FCraftingRequest ActiveRequest;

	UPROPERTY(BlueprintReadOnly, Category="State")
	FSkillCheckResult ActiveCheck;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AActor> CraftInstigator;
	FTimerHandle CraftTimer;

	UInventoryComponent* ResolveOutputInventory(AActor* Instigator) const;
	void GrantOutputs(const TArray<FCraftItemOutput>& Outputs, int32 QualityTier, AActor* Instigator);
	void GiveStartXPIfAny(const FCraftingRequest& Req, AActor* Instigator);
	void GiveFinishXPIfAny(const FCraftingRequest& Req, AActor* Instigator, bool bSuccess);

	bool IsPresenceSatisfied(AActor* Instigator, const FCraftingRequest& Req) const;
	void FinishCraft();

	static void GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out);
};
