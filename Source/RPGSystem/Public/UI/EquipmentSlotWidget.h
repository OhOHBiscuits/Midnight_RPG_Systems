#pragma once

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EquipmentSlotWidget.generated.h"

class UImage;
class UEquipmentComponent;
class UItemDataAsset;
class UInventoryDragDropOp;

UCLASS()
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which slot does this widget represent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|UI")
	FGameplayTag SlotTag;

	/** Optional: auto-bind to local player’s PS equipment at construct */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|UI")
	bool bAutoBindEquipment = true;

	/** Optional icon brush receiver (hook this in UMG) */
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	UImage* IconImage = nullptr;

	/** Manual binding API if you don’t use auto-bind */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|UI")
	void BindToEquipment(UEquipmentComponent* Equip);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|UI")
	void RefreshVisuals();

protected:
	UPROPERTY()
	TWeakObjectPtr<UEquipmentComponent> BoundEquip;

	// Events from component
	UFUNCTION()
	void HandleEquipChanged(FGameplayTag InSlot, UItemDataAsset* InData);

	UFUNCTION()
	void HandleSlotCleared(FGameplayTag InSlot);

	// UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Drag & Drop
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// For building drag payloads in Blueprints if desired
	UFUNCTION(BlueprintNativeEvent, Category="1_Equipment|UI")
	bool BuildDragOperation(UInventoryDragDropOp*& OutOperation);
	virtual bool BuildDragOperation_Implementation(UInventoryDragDropOp*& OutOperation) { OutOperation = nullptr; return false; }

private:
	void ApplyIcon(UItemDataAsset* Data);
};
