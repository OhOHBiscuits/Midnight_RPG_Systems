#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Actors/BaseWorldItemActor.h"
#include "WorkstationActor.generated.h"

class UUserWidget;

/**
 * Base workstation actor (e.g., forge, oven, etc.)
 * - Shares the unified world-item UI pipeline from ABaseWorldItemActor.
 * - Child classes may override OpenWorkstationUIFor for custom preconditions.
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

	/** Called server-side when we want to open the workstation UI for an interactor. */
	UFUNCTION(BlueprintCallable, Category="Workstation")
	virtual void OpenWorkstationUIFor(AActor* Interactor);

protected:
	/** Server-side interaction entry from the base class. */
	virtual void HandleInteract_Server(AActor* Interactor) override;

	/** Anchor only for stations that show extra visuals (VFX, temp meshes, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Visuals")
	USceneComponent* VisualsRoot;

	/** Optional “currently processed item” preview mesh (hidden by default). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation|Visuals")
	UStaticMeshComponent* PreviewMesh;


	/** Simple hook you can call from BP when input changes, etc. */
	UFUNCTION(BlueprintCallable, Category="Workstation|Visuals")
	void HidePreview();
};
