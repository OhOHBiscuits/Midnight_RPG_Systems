#pragma once

#include "CoreMinimal.h"
#include "Actors/BaseWorldItemActor.h"
#include "StorageActor.generated.h"

class UInventoryComponent;
class UUserWidget;

UCLASS()
class RPGSYSTEM_API AStorageActor : public ABaseWorldItemActor
{
	GENERATED_BODY()
public:
	AStorageActor();

	/** Storage inventory component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Storage")
	UInventoryComponent* InventoryComp;

	/** UI to open when interacting (fallback to WidgetClass if null) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storage|UI")
	TSubclassOf<UUserWidget> StorageWidgetClass;

	/** Blueprint helper if you want to trigger UI from BP */
	UFUNCTION(BlueprintCallable, Category="Storage|UI")
	void OpenStorageUIFor(AActor* Interactor);

protected:
	virtual void BeginPlay() override;
	virtual void HandleInteract_Server(AActor* Interactor) override;
};
