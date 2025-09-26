#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryPanelWidget.generated.h"

class UPanelWidget;
class UInventoryComponent;
class UItemDataAsset;
class UInventoryItemSlotWidget;

/** Reusable inventory view. Only rebuild/refresh on change. */
UCLASS(Blueprintable)
class RPGSYSTEM_API UInventoryPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The inventory shown by this panel. Exposed on spawn + BP getter/setter nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup", meta=(ExposeOnSpawn="true"))
	TObjectPtr<UInventoryComponent> InventoryRef = nullptr;

	/** Container the panel will populate with slot widgets (WrapBox/UniformGrid/etc.). */
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="1_Inventory-UI|Setup")
	TObjectPtr<UPanelWidget> SlotContainer = nullptr;

	/** Slot widget class to spawn for each slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup")
	TSubclassOf<UInventoryItemSlotWidget> SlotWidgetClass;

	/** Rebuild all on Construct if InventoryRef is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Setup")
	bool bAutoBindOnConstruct = true;

	/** Optional throttle; if false, we rebuild immediately when counts change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-UI|Perf")
	bool bDeferFullRebuild = false;

	// ----- Blueprint accessors (show up as nodes) -----
	UFUNCTION(BlueprintGetter, Category="1_Inventory-UI|Setup")
	UInventoryComponent* GetInventoryRef() const { return InventoryRef; }

	UFUNCTION(BlueprintSetter, Category="1_Inventory-UI|Setup")
	void SetInventoryRef(UInventoryComponent* InInventory) { InitializeWithInventory(InInventory); }

	// ----- External API -----
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void InitializeWithInventory(UInventoryComponent* InInventory);

	/** If you don't want to name the container "SlotContainer", set it explicitly. */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	void SetSlotContainer(class UPanelWidget* InContainer) { SlotContainer = InContainer; }

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Refresh")
	void RefreshAll();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Refresh")
	void RefreshSlot(int32 SlotIndex);

	/** BP hook before/after rebuild to adjust layout, headers, etc. */
	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnAboutToRebuild();

	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-UI|Events")
	void OnFinishedRebuild();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Inventory events
	UFUNCTION() void HandleSlotUpdated(int32 SlotIndex);
	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void HandleWeightChanged(float NewW);
	UFUNCTION() void HandleVolumeChanged(float NewV);

private:
	void BindInventory(UInventoryComponent* InInventory);
	void UnbindInventory();

	/** Ensures SlotContainer has exactly N children; spawns & binds each BEFORE Construct. */
	void EnsureSlotWidgets(int32 DesiredCount);

	/** Returns (ItemData, Qty) for a slot; null if empty. */
	void QuerySlotData(int32 SlotIndex, UItemDataAsset*& OutData, int32& OutQty) const;
};
