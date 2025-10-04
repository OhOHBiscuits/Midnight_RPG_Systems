#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryPanelWidget.generated.h"

class UPanelWidget;
class UInventoryComponent;
class UItemDataAsset;
class UInventoryItemSlotWidget;

/** Reusable inventory view; spawns/binds slot widgets and listens for component events. */
UCLASS(Blueprintable)
class RPGSYSTEM_API UInventoryPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Expose on spawn + getter/setter nodes for BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter=GetInventoryRef, BlueprintSetter=SetInventoryRef, Category="1_Inventory-UI|Setup", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="1_Inventory-UI|Setup")
	TObjectPtr<UPanelWidget> SlotContainer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup")
	TSubclassOf<UInventoryItemSlotWidget> SlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup")
	bool bAutoBindOnConstruct = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Perf")
	bool bDeferFullRebuild = false;

	/** Drop onto empty panel area to auto-place (stack first, then empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Input")
	bool bAcceptDropsOnPanel = true;

	// Getter/Setter for BP nodes
	UFUNCTION(BlueprintGetter, Category="1_Inventory-UI|Setup")
	UInventoryComponent* GetInventoryRef() const { return InventoryRef; }

	UFUNCTION(BlueprintSetter, Category="1_Inventory-UI|Setup")
	void SetInventoryRef(UInventoryComponent* InInventory) { InitializeWithInventory(InInventory); }

	// External API
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void InitializeWithInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void SetSlotContainer(class UPanelWidget* InContainer) { SlotContainer = InContainer; }

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Refresh")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Refresh")
	void RefreshSlot(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnAboutToRebuild();

	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnFinishedRebuild();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;

	// Inventory events
	UFUNCTION() void HandleSlotUpdated(int32 SlotIndex);
	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void HandleWeightChanged(float NewW);
	UFUNCTION() void HandleVolumeChanged(float NewV);

private:
	void BindInventory(UInventoryComponent* InInventory);
	void UnbindInventory();
	void EnsureSlotWidgets(int32 DesiredCount);
	void QuerySlotData(int32 SlotIndex, UItemDataAsset*& OutData, int32& OutQty) const;
	bool TryAutoPlaceDrag(class UInventoryComponent* TargetInv, class UDragDropOperation* Op);
};
