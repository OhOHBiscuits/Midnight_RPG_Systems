#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOp.generated.h"

class UInventoryComponent;
class UInventoryPanelWidget;

/** Drag payload for inventory items. */
UCLASS(BlueprintType)
class RPGSYSTEM_API UInventoryDragDropOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** Where the item is currently coming from. */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	/** Index within SourceInventory. */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	int32 FromIndex = INDEX_NONE;

	/** Optional partial-stack amount; <=0 = full stack. */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	int32 Quantity = 0;

	/** So the target can refresh the source panel immediately after drop. */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	TWeakObjectPtr<UInventoryPanelWidget> SourcePanel;
};
