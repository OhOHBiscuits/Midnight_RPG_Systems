// BaseWorldItemActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/InteractionInterface.h"
#include "Inventory/ItemDataAsset.h"
#include "BaseWorldItemActor.generated.h"

class UStaticMeshComponent;
class UUserWidget;

UCLASS(Blueprintable)
class RPGSYSTEM_API ABaseWorldItemActor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	ABaseWorldItemActor();

	/** Static mesh shown in world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldItem")
	UStaticMeshComponent* Mesh;

	/** Source data for visuals/behavior */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="WorldItem")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	/** Optional global UI to pop on interact (storage/workstations will override or provide their own) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WorldItem|UI")
	TSubclassOf<UUserWidget> WidgetClass;

	/** If true, EfficiencyRating matters for this actor type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem")
	bool bUseEfficiency = false;

	/** Current efficiency (often copied from ItemData) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem")
	float EfficiencyRating = 1.f;

	/** IInteractionInterface – implemented in C++ and callable from BP */
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FGameplayTag GetInteractionTypeTag_Implementation() const override { return FGameplayTag(); }

	/** Show a UI on the owning client (helper you can call from children) */
	UFUNCTION(BlueprintCallable, Category="WorldItem|UI")
	void ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

protected:
	// Replication
	virtual void GetLifetimeReplicatedProps(
	   TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Construction/Editor QoL – keep visuals in sync
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	UFUNCTION()
	void OnRep_ItemData();

	/** Apply ItemData-driven visuals (mesh, etc) */
	void ApplyItemDataVisuals();

	/** Client RPC to actually create the widget */
	UFUNCTION(Client, Reliable)
	void Client_ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

	/** Server RPC entry for interactions */
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Interactor);

	/** What children actually do on the server when interacted with */
	virtual void HandleInteract_Server(AActor* Interactor) {}
};
