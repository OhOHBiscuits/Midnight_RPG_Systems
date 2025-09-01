#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Actors/BaseWorldItemActor.h"
#include "WorkstationActor.generated.h"

class UUserWidget;
class UCraftingStationComponent;
class UCraftingQueueComponent;
class UCraftingRecipeDataAsset;
struct FCraftingRequest;
struct FSkillCheckResult;

/**
 * Base workstation actor (forge, oven, table, etc.)
 * Keeps your world-item UI + adds Crafting Station & Queue (optional use).
 */
UCLASS()
class RPGSYSTEM_API AWorkstationActor : public ABaseWorldItemActor
{
	GENERATED_BODY()

public:
	AWorkstationActor();

	/** The widget opened when a player interacts with this workstation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation|UI")
	TSubclassOf<UUserWidget> WorkstationWidgetClass;

	/** Optional: a gameplay tag to identify this workstation’s interaction type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation")
	FGameplayTag InteractionTypeTag;

	/** Crafting Station (server-authoritative). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Crafting-Setup")
	UCraftingStationComponent* CraftingStation;
	
	/** Called server-side when we want to open the workstation UI for an interactor. */
	UFUNCTION(BlueprintCallable, Category="Workstation")
	virtual void OpenWorkstationUIFor(AActor* Interactor);

	// ---------- Crafting helpers (Blueprint-friendly) ----------
	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions")
	bool StartCraftFromRecipe(UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions")
	int32 EnqueueRecipeBatch(UCraftingRecipeDataAsset* Recipe, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions")
	void CancelCraft();

protected:
	/** Server-side interaction entry from the base class. */
	virtual void HandleInteract_Server(AActor* Interactor) override;

	/** Anchor for visuals (preview mesh, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Visuals")
	USceneComponent* VisualsRoot;

	/** Optional “currently processed item” preview mesh (hidden by default). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Visuals")
	UStaticMeshComponent* PreviewMesh;

	// Station event sinks (forwarded for convenience)
	UFUNCTION()
	void HandleCraftStarted(const FCraftingRequest& Request, float FinalTime, const FSkillCheckResult& Check);

	UFUNCTION()
	void HandleCraftCompleted(const FCraftingRequest& Request, const FSkillCheckResult& Check, bool bSuccess);

	/** Simple hook you can call from BP when input changes, etc. */
	UFUNCTION(BlueprintCallable, Category="Workstation|Visuals")
	void HidePreview();
};
