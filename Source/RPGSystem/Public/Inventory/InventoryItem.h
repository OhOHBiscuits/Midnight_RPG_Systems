#pragma once

#include "CoreMinimal.h"
#include "InventoryItem.generated.h"

class UItemDataAsset;

/**
 * A single stack of items in an inventory slot at runtime.
 */
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	/** Reference to the static item definition asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	/** How many items are in this stack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 1;

	/** Which slot this item occupies (for UI convenience) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = -1;

	/** Returns true if this struct currently holds a valid item */
	bool IsValid() const
	{
		return ItemData.IsValid();
	}

	/** Returns true if this item can stack (MaxStackSize > 1) */
	bool IsStackable() const;
};
