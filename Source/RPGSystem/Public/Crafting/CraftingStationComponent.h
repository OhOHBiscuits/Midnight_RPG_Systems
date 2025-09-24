// CraftingStationComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Crafting/CraftingTypes.h"
#include "CraftingStationComponent.generated.h"

class UCraftingRecipeDataAsset;
class UInventoryComponent;
class AWorkstationActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCraftingStartedSignature, const FCraftingJob&, Job);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCraftingFinishedSignature, const FCraftingJob&, Job, bool, bSuccess);

UCLASS(ClassGroup=(RPGSystem), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCraftingStationComponent();

	// Inventories
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory-Crafting|Inventories")
	TObjectPtr<UInventoryComponent> InputInventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Inventory-Crafting|Inventories")
	TObjectPtr<UInventoryComponent> OutputInventory = nullptr;

	// Replicated state
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Crafting|State")
	FCraftingJob ActiveJob;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Crafting|State")
	bool bIsCrafting = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Crafting|State")
	bool bIsPaused = false;

	/** Resolve recipe inputs/outputs to items by tag at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Crafting|Tags")
	bool bResolveItemsByTag = true;

	// API
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Crafting|Actions")
	bool StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe, int32 Times = 1);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Crafting|Actions")
	void CancelCraft();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Crafting|Actions")
	void PauseCraft();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Crafting|Actions")
	void ResumeCraft();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Crafting|Utility")
	void GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const;

	// Events
	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Crafting|Events")
	FCraftingStartedSignature OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Crafting|Events")
	FCraftingFinishedSignature OnCraftFinished;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Finishes the current craft tick. */
	void FinishCraft_Internal();

	// Inventory hooks
	bool MoveRequiredToInput(const UCraftingRecipeDataAsset* Recipe, int32 Times, AActor* InstigatorActor);
	void DeliverOutputs(const UCraftingRecipeDataAsset* Recipe);
	void GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* Recipe, AActor* InstigatorActor, bool bSuccess);

private:
	FTimerHandle CraftTimerHandle;

	bool HasAuth() const
	{
		const AActor* Owner = GetOwner();
		return Owner && Owner->HasAuthority();
	}
};
