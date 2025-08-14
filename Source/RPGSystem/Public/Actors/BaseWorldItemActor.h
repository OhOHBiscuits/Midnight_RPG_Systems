#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Interfaces/InteractionInterface.h"
#include "GameplayTagContainer.h"
#include "BaseWorldItemActor.generated.h"

class UStaticMeshComponent;
class UUserWidget;
class UItemDataAsset;

/**
 * Common base for world items (pickup, storage, workstation).
 * Handles: replicated ItemData, auto-updating mesh in editor/PIE,
 * generic Interact flow (client->server), and UI display to owning client.
 */
UCLASS(BlueprintType, Blueprintable)
class RPGSYSTEM_API ABaseWorldItemActor : public AActor
{
	GENERATED_BODY()

public:
	ABaseWorldItemActor();

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldItem")
	UStaticMeshComponent* Mesh;

	// ---- Data/Visuals ----
	/** Item data that drives visuals (mesh), decay, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="WorldItem|Data")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	/** If true, this world actor leverages the data's EfficiencyRating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem|Data")
	bool bUseEfficiency = false;

	/** Per-instance efficiency value (initialized from ItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem|Data")
	float EfficiencyRating = 1.0f;

	/** Default/fallback UI for this actor when interacted with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WorldItem|UI")
	TSubclassOf<UUserWidget> WidgetClass;

	// ---- Interaction (Blueprint-native so BPs can call 'Interact') ----
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor);

	/** Optional interaction type for UI prompts, etc. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FGameplayTag GetInteractionTypeTag() const;
	virtual FGameplayTag GetInteractionTypeTag_Implementation() const
	{
		return FGameplayTag(); // default: None
	}

	// ---- UI helper (call from server or client) ----
	UFUNCTION(BlueprintCallable, Category="WorldItem|UI")
	void ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

protected:
	// Server RPC that executes the actual interaction behavior.
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Interactor);
	virtual void Server_Interact_Implementation(AActor* Interactor);

	// Client RPC: actually spawns UI on owning local player.
	UFUNCTION(Client, Reliable)
	void Client_ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);
	virtual void Client_ShowWorldItemUI_Implementation(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

	/** Override this on children to perform the real work on the server */
	virtual void HandleInteract_Server(AActor* Interactor);

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Keep visuals in sync in-editor & at runtime
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	void OnRep_ItemData();

	/** Apply ItemData-driven visuals (mesh, efficiency, etc.) */
	void ApplyItemDataVisuals();

	
};
