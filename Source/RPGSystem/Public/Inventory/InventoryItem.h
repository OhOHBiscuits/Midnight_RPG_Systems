#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemDataAsset.h"
#include "InventoryItem.generated.h"

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	FInventoryItem()
		: ItemData(nullptr)
		, Quantity(0)
		, ItemIndex(-1)
	{}

	FInventoryItem(UItemDataAsset* InItemData, int32 InQuantity, int32 InItemIndex = -1)
		: ItemData(InItemData)
		, Quantity(InQuantity)
		, ItemIndex(InItemIndex)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;

	/** The index this item occupies in the inventory array, for UI or reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemIndex;

	// Utility: Is this slot valid/occupied?
	bool IsValid() const
	{
		return ItemData.IsValid() && Quantity > 0;
	}

	// (Optional) Is this item stackable? (Uses ItemData settings if present)
	bool IsStackable() const
	{
		if (ItemData.IsValid())
		{
			UItemDataAsset* Asset = ItemData.Get();
			return Asset && Asset->MaxStackSize > 1;
		}
		return false;
	}
};
