#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CraftingComponent.generated.h"

class UInventoryComponent;
class UFuelComponent;
class UCraftingRecipeDataAsset;

// Events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingStarted, FGameplayTag, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCraftingProgress, FGameplayTag, RecipeID, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingCompleted, FGameplayTag, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingCancelled, FGameplayTag, RecipeID);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCraftingComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting")
	UInventoryComponent* InputInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting")
	UInventoryComponent* OutputInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting")
	UFuelComponent* FuelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting")
	FGameplayTag StationTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting")
	float CraftSpeedMultiplier = 1.0f;

	// Area sourcing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area")
	bool bUseNearbyPublicInventories = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area", meta=(EditCondition="bUseNearbyPublicInventories", ClampMin="100.0"))
	float PublicInventorySearchRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area", meta=(EditCondition="bUseNearbyPublicInventories"))
	FGameplayTag PublicStorageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area")
	bool bIncludeInteractorInventory = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Crafting|Area")
	bool bFallbackOutputToPublicStorage = true;

	// Replicated state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting")
	bool bIsCrafting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting")
	FGameplayTag ActiveRecipeID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="1_Inventory|Crafting")
	float RemainingSeconds = 0.f;

	// Events
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting")
	FCraftingStarted OnCraftingStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting")
	FCraftingProgress OnCraftingProgress;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting")
	FCraftingCompleted OnCraftingCompleted;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Crafting")
	FCraftingCancelled OnCraftingCancelled;

	// BP API (client-callable; runs on server)
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting")
	bool TryStartCraft(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount = 1, AActor* Interactor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting")
	void TryCancelCraft();

	// Queries
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool CanCraft(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool HasRequiredInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool HasOutputSpace(UCraftingRecipeDataAsset* Recipe, AActor* Interactor = nullptr) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool IsFuelSatisfied(UCraftingRecipeDataAsset* Recipe) const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY()
	UCraftingRecipeDataAsset* ActiveRecipe = nullptr;

	UPROPERTY(Replicated)
	int32 RemainingRepeats = 0;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedInteractor;

	FTimerHandle CraftTimer;

	// Core
	bool StartCraft_Internal(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor);
	void StartCraftTimer(UCraftingRecipeDataAsset* Recipe);
	void ClearCraftState(bool bBroadcastCancel);

	// IO helpers
	void GatherInputSources(AActor* Interactor, TArray<UInventoryComponent*>& OutSources) const;
	UInventoryComponent* SelectOutputInventory(AActor* Interactor) const;

	bool RemoveFromSources(const FGameplayTag& ItemID, int32 Quantity, const TArray<UInventoryComponent*>& Sources);
	int32 CountAcrossSources(const FGameplayTag& ItemID, const TArray<UInventoryComponent*>& Sources) const;

	void ConsumeInputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor);
	void GrantOutputs(UCraftingRecipeDataAsset* Recipe, AActor* Interactor);

	UFUNCTION()
	void CraftTick();

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartCraft(UCraftingRecipeDataAsset* Recipe, int32 RepeatCount, AActor* Interactor);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCancelCraft();
};
