#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseInventoryWidget.generated.h"

class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotWidgetUpdate, int32, SlotIndex);

UCLASS()
class RPGSYSTEM_API UBaseInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// The inventory being displayed
	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	UInventoryComponent* LinkedInventory = nullptr;

	// BP can bind to this to update a single slot UI
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnSlotWidgetUpdate OnSlotWidgetUpdate;

	// BP: Rebuild grid/list/equipment UI from scratch (flexible for any inventory behavior)
	UFUNCTION(BlueprintImplementableEvent, Category="Inventory")
	void RebuildInventoryGrid(UInventoryComponent* Inventory);

	// Set which inventory to show, binds delegates
	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual void SetLinkedInventory(UInventoryComponent* Inventory);

	// Drag/drop between slots (same inventory, grid/list/equipment) - all C++ logic
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void HandleDragDrop(int32 FromSlot, int32 ToSlot);

	// Transfer between inventories (player/storage/workstation) - all C++ logic
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void RequestItemTransfer(
		UInventoryComponent* SourceInventory, int32 SourceSlot,
		UInventoryComponent* TargetInventory, int32 TargetSlot, int32 Quantity);

	// Called when a single slot is updated by the inventory component
	UFUNCTION()
	virtual void OnInventorySlotUpdated(int32 SlotIndex);

	// Called when the whole inventory is rebuilt (e.g. sort, clear, hotbar/bag switch)
	UFUNCTION()
	virtual void OnInventoryChanged();

protected:
	virtual void NativeDestruct() override;
};
