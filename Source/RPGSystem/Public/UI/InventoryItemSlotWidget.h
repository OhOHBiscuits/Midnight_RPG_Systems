#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemSlotWidget.generated.h"

class UInventoryComponent;
class UItemDataAsset;
class UInventoryDragDropOp;
class UInventoryPanelWidget;

/** One visual slot; binds to an inventory/index and self-updates. */
UCLASS(Blueprintable, Abstract)
class RPGSYSTEM_API UInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Optional: set on CreateWidget in BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup", meta=(ExposeOnSpawn="true"))
	int32 SlotIndex = INDEX_NONE;

	/** Current visual state (read-only for UMG). */
	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	TObjectPtr<UItemDataAsset> ItemData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="1_Inventory-UI|State")
	int32 Quantity = 0;

	/** Drag config. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	FKey DragMouseButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	bool bAllowDragWhenEmpty = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	TSubclassOf<UInventoryDragDropOp> DragOpClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	TSubclassOf<UUserWidget> DragVisualClass;

	/** Right-click menu hook. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Input")
	bool bConsumeRightClick = true;

	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Input")
	void OnSlotRightClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent);

	/** After a successful drop, choose whether to rebuild whole panel(s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Perf")
	bool bFullRefreshAfterDropSameInventory = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Perf")
	bool bFullRefreshAfterDropCrossInventory = true;

	// ----- BP accessors -----
	UFUNCTION(BlueprintPure,  Category="1_Inventory-UI|State") UInventoryComponent* GetSlotInventory() const { return InventoryRef; }
	UFUNCTION(BlueprintPure,  Category="1_Inventory-UI|State") int32                GetSlotIndex()     const { return SlotIndex; }
	UFUNCTION(BlueprintPure,  Category="1_Inventory-UI|State") UItemDataAsset*      GetSlotItemData()  const { return ItemData; }
	UFUNCTION(BlueprintPure,  Category="1_Inventory-UI|State") int32                GetSlotQuantity()  const { return Quantity; }

	/** Your BP paints icon/qty/name here after state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnSlotVisualsChanged();

	// ----- Wiring & refresh -----
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void BindToInventory(UInventoryComponent* InInv, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void UnbindFromInventory();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Refresh")
	void UpdateSlotFromInventory();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|State")
	void SetSlotData(UInventoryComponent* InInv, int32 InIndex, UItemDataAsset* InData, int32 InQty)
	{
		InventoryRef = InInv; SlotIndex = InIndex; ItemData = InData; Quantity = InQty;
		OnSlotVisualsChanged();
	}

protected:
	UInventoryItemSlotWidget(const FObjectInitializer&);

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// Native drag pipeline
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent) override;
	virtual void   NativeOnDragDetected   (const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool   NativeOnDrop           (const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Per-slot policy. */
	UFUNCTION(BlueprintNativeEvent, Category="1_Inventory-UI|Drop")
	bool CanAcceptDrop(const UInventoryDragDropOp* DragOp) const;
	virtual bool CanAcceptDrop_Implementation(const UInventoryDragDropOp* DragOp) const { return true; }

	/** Let BP fully craft the drag operation; return true to use it. */
	UFUNCTION(BlueprintNativeEvent, Category="1_Inventory-UI|Drag")
	bool BuildDragOperation(UInventoryDragDropOp*& OutOperation);

	/** Let BP (or default) build the drag visual widget. */
	UFUNCTION(BlueprintNativeEvent, Category="1_Inventory-UI|Drag")
	UUserWidget* CreateDragVisual();

private:
	UFUNCTION() void HandleInvSlotUpdated(int32 UpdatedIndex);
	UFUNCTION() void HandleInvChanged();

	void UpdateFromInventory();
	UItemDataAsset* ResolveItemData() const;

	TWeakObjectPtr<UInventoryPanelWidget> CachedPanel;
};
