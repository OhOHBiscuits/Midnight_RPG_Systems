#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingComponent.generated.h"

class UInventoryComponent;
class UFuelComponent;
class UCraftingRecipeDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingStarted, FGameplayTag, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCraftingProgress, FGameplayTag, RecipeID, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingCompleted, FGameplayTag, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingCancelled, FGameplayTag, RecipeID);

// Fires when level is too low
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCraftingProficiencyFailed, FGameplayTag, SkillTag, int32, RequiredLevel, int32, CurrentLevel);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCraftingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting") UInventoryComponent* InputInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting") UInventoryComponent* OutputInventory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting") UFuelComponent* FuelComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting") FGameplayTag StationTypeTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting") float CraftSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area") bool bUseNearbyPublicInventories = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area", meta=(EditCondition="bUseNearbyPublicInventories", ClampMin="100.0")) float PublicInventorySearchRadius = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area", meta=(EditCondition="bUseNearbyPublicInventories")) FGameplayTag PublicStorageTypeTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area") bool bIncludeInteractorInventory = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area") bool bFallbackOutputToPublicStorage = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting") bool bIsCrafting = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting") FGameplayTag ActiveRecipeID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting") float RemainingSeconds = 0.f;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting") FCraftingStarted OnCraftingStarted;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting") FCraftingProgress OnCraftingProgress;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting") FCraftingCompleted OnCraftingCompleted;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting") FCraftingCancelled OnCraftingCancelled;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting") FCraftingProficiencyFailed OnCraftingProficiencyFailed;

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting")
	bool TryStartCraft(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount = 1, AActor* Interactor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting")
	void TryCancelCraft();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool CanCraft(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool HasRequiredInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool HasOutputSpace(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool IsFuelSatisfied(UCraftingRecipeDataAsset* Recipe) const;

	// Proficiency helpers
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Crafting|Proficiency")
	int32 GetInteractorProficiency(AActor* Interactor, const FGameplayTag& SkillTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Crafting|Proficiency")
	bool IsProficiencySatisfied(UCraftingRecipeDataAsset* Recipe, AActor* Interactor, int32& OutCurrentLevel) const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY() UCraftingRecipeDataAsset* ActiveRecipe = nullptr;
	UPROPERTY(Replicated) int32 RemainingRepeats = 0;
	UPROPERTY() TWeakObjectPtr<AActor> CachedInteractor;
	FTimerHandle CraftTimer;

	bool StartCraft_Internal(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor);
	void StartCraftTimer(UCraftingRecipeDataAsset* Recipe, int32 InteractorLevel);
	void ClearCraftState(bool bBroadcastCancel);

	void GatherInputSources(AActor* Interactor, TArray<UInventoryComponent*>& OutSources) const;
	UInventoryComponent* SelectOutputInventory(AActor* Interactor) const;

	bool RemoveFromSources(const FGameplayTag& ItemID, int32 Quantity, const TArray<UInventoryComponent*>& Sources);
	int32 CountAcrossSources(const FGameplayTag& ItemID, const TArray<UInventoryComponent*>& Sources) const;

	void ConsumeInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor);
	void GrantOutputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor, int32 InteractorLevel);

	UFUNCTION() void CraftTick();

	UFUNCTION(Server, Reliable, WithValidation) void ServerStartCraft(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerCancelCraft();
};
