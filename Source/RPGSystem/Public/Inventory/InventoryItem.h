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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemIndex;

	/** Consider a slot valid if it has a non-null soft reference and positive quantity (works in Standalone even if not loaded). */
	bool IsValid() const
	{
		return !ItemData.IsNull() && Quantity > 0;
	}

	/** True if data says it can stack (safe if asset isn’t loaded yet). */
	bool IsStackable() const
	{
		if (ItemData.IsNull()) return false;
		const UItemDataAsset* Asset = ItemData.Get();
		return Asset && Asset->MaxStackSize > 1;
	}

	FORCEINLINE FGameplayTag GetItemID() const
	{
		return (!ItemData.IsNull() && ItemData.Get()) ? ItemData->ItemIDTag : FGameplayTag();
	}

	/** Use this only when you truly need the asset; will sync‑load in Standalone if not resident. */
	UItemDataAsset* ResolveItemData() const
	{
		if (ItemData.IsNull()) return nullptr;
		UItemDataAsset* Data = ItemData.Get();
		if (!Data && ItemData.ToSoftObjectPath().IsValid())
		{
			Data = ItemData.LoadSynchronous();
		}
		return Data;
	}
};
