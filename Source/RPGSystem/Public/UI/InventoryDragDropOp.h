#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOp.generated.h"

class UInventoryComponent;

/** Drag payload for inventory items. */
UCLASS(BlueprintType)
class RPGSYSTEM_API UInventoryDragDropOp : public UDragDropOperation
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	int32 FromIndex = INDEX_NONE;

	/** Optional: number to split/move (<= stack size). If <=0, move all. */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-UI|Drag")
	int32 Quantity = 0;
};
