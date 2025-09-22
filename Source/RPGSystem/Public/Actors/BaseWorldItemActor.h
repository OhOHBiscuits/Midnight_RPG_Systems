#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "BaseWorldItemActor.generated.h"

class UItemDataAsset;
class UStaticMeshComponent;
class UUserWidget;

UCLASS(Blueprintable)
class RPGSYSTEM_API ABaseWorldItemActor : public AActor
{
	GENERATED_BODY()

public:
	ABaseWorldItemActor();

	/** Soft reference to the data row/asset that drives visuals & behavior */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="WorldItem")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	/** World mesh used when the item is placed or dropped */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldItem")
	UStaticMeshComponent* Mesh = nullptr;

	/** Optional: use instance efficiency (mirrors ItemData->EfficiencyRating by default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem")
	bool bUseEfficiency = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="WorldItem", meta=(EditCondition="bUseEfficiency"))
	float EfficiencyRating = 1.f;

	// Interact entry point (Interface-style, already networked)
	UFUNCTION(BlueprintNativeEvent, Category="WorldItem|Interact")
	void Interact(AActor* Interactor);
	virtual void Interact_Implementation(AActor* Interactor);

	/** Server side interaction */
	UFUNCTION(Server, Reliable)
	void Server_Interact(AActor* Interactor);
	void Server_Interact_Implementation(AActor* Interactor);

	/** Spawn a UI on the owning client */
	UFUNCTION(Client, Reliable)
	void Client_ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);
	void Client_ShowWorldItemUI_Implementation(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

	/** Helper that routes to client correctly */
	void ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	void OnRep_ItemData();

	/** ⬅️ Make this virtual so child actors (weapons) can override cleanly */
	virtual void ApplyItemDataVisuals();

	/** Override point for children that run on the server when Interact is triggered */
	virtual void HandleInteract_Server(AActor* Interactor);

private:
	void ResolveOwningPCForUI(AActor* Interactor, APlayerController*& OutPC) const;
};
