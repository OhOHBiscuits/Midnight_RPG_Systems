#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "InventoryItem.generated.h"

class UItemDataAsset;

USTRUCT(BlueprintType)
struct RPGSYSTEM_API FInventoryItem
{
	GENERATED_BODY()

public:
	// Data + state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="0"))
	int32 Quantity = 0;

	/** Runtime convenience (not saved) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
	int32 Index = INDEX_NONE;

	// --- C++ helpers (no UFUNCTION inside USTRUCT) ---
	FORCEINLINE bool IsValid() const
	{
		return ItemData.IsValid() && Quantity > 0;
	}

	FORCEINLINE UItemDataAsset* ResolveItemData() const
	{
		return ItemData.IsNull() ? nullptr : ItemData.LoadSynchronous();
	}

	FORCEINLINE bool CanStackWith(UItemDataAsset* Other) const
	{
		const UItemDataAsset* Self = ItemData.Get();
		if (!Self || !Other) return false;
		if (!Self->bStackable || !Other->bStackable) return false;
		return Self->ItemIDTag == Other->ItemIDTag;
	}
};
