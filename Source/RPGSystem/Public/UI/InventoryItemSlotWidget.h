#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemSlotWidget.generated.h"

class UInventoryComponent;
class UItemDataAsset;
class UInventoryDragDropOp;
class UInventoryPanelWidget;

/** One visual slot. Panel owns layout; slot binds to its inventory and self-updates. */
UCLASS(Blueprintable, Abstract)
class RPGSYSTEM_API UInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	TObjectPtr<UItemDataAsset> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	int32 Quantity = 0;

	/** Paint your icon/stack/etc. in BP after the data changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnSlotVisualsChanged();

	/** Panel calls this to wire the slot to a specific inventory/index. */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void BindToInventory(UInventoryComponent* InInv, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void UnbindFromInventory();

	/** Useful for previews in BP if needed. Also used internally after queries. */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|State")
	void SetSlotData(UInventoryComponent* InInv, int32 InIndex, UItemDataAsset* InData, int32 InQty)
	{
		InventoryRef = InInv; SlotIndex = InIndex; ItemData = InData; Quantity = InQty;
		OnSlotVisualsChanged();
	}

protected:
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	/** Gate for per-panel rules (e.g., Input-only accepts mats). */
	UFUNCTION(BlueprintNativeEvent, Category="1_Inventory-UI|Drop")
	bool CanAcceptDrop(const UInventoryDragDropOp* DragOp) const;
	virtual bool CanAcceptDrop_Implementation(const UInventoryDragDropOp* DragOp) const { return true; }

private:
	UFUNCTION() void HandleInvSlotUpdated(int32 UpdatedIndex);
	UFUNCTION() void HandleInvChanged();

	/** Pulls data from InventoryRef/SlotIndex and calls SetSlotData. */
	void UpdateFromInventory();

	/** Hard-resolves a soft item reference for UI. */
	UItemDataAsset* ResolveItemData() const;

	/** Cached pointer to parent panel for quick refresh after intra-inventory moves (optional). */
	TWeakObjectPtr<UInventoryPanelWidget> CachedPanel;
};
