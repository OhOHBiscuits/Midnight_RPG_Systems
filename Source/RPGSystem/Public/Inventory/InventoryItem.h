#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/Guid.h"
#include "ItemDataAsset.h"
#include "InventoryItem.generated.h"


USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

public:
	FInventoryItem()
		: ItemData(nullptr)
		, Quantity(0)
		, ItemIndex(INDEX_NONE)
	{}

	FInventoryItem(UItemDataAsset* InItemData, int32 InQuantity, int32 InItemIndex = INDEX_NONE)
		: ItemData(InItemData)
		, Quantity(InQuantity)
		, ItemIndex(InItemIndex)
	{}

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Quantity;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ItemIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	bool bIsHeirloom = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	FString OwnerPlayerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	FGuid UniqueInstanceId;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	FText CustomDisplayName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	bool bNonTransferable = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Heirloom")
	bool bNonDestructible = false;
		
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid InstanceGuid;
    
	/** Convenience */
	bool IsOwnedBy(const FString& PlayerId) const
	{
		return bIsHeirloom && !OwnerPlayerId.IsEmpty()
			&& OwnerPlayerId.Equals(PlayerId, ESearchCase::IgnoreCase);
	}

	
	bool IsValid() const
	{
		
		return !ItemData.IsNull() && Quantity > 0;
	}

	
	bool IsStackable() const
	{
		
		if (bIsHeirloom) return false;

		if (ItemData.IsNull()) return false;
		const UItemDataAsset* Asset = ItemData.Get();
		return Asset && Asset->MaxStackSize > 1;
	}

	
	FORCEINLINE FGameplayTag GetItemID() const
	{
		return (!ItemData.IsNull() && ItemData.Get()) ? ItemData->ItemIDTag : FGameplayTag();
	}

	
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

	
	FText GetDisplayName() const
	{
		if (!CustomDisplayName.IsEmpty())
		{
			return CustomDisplayName;
		}
		if (const UItemDataAsset* Data = ItemData.Get())
		{
			return Data->Name;
		}
		return FText::GetEmpty();
	}

	/** True if this heirloom is owned by the provided player/account id. */
	bool IsHeirloomOwnedBy(const FString& InOwnerId) const
	{
		return bIsHeirloom && !OwnerPlayerId.IsEmpty() && OwnerPlayerId.Equals(InOwnerId, ESearchCase::IgnoreCase);
	}

	/** Can this item be transferred to another inventory given the requestor’s account id? */
	bool CanTransferTo(const FString& RequestorOwnerId, bool bTargetIsPrivateInventoryOfOwner) const
	{
		if (!IsValid()) return false;

		// Non-transferable items can never move out of their current inventory,
		// unless the target is the owner's private inventory (optional carve-out).
		if (bNonTransferable)
		{
			return bTargetIsPrivateInventoryOfOwner && IsHeirloomOwnedBy(RequestorOwnerId);
		}

		// Heirlooms may only move if the requestor is the bound owner.
		if (bIsHeirloom)
		{
			return IsHeirloomOwnedBy(RequestorOwnerId);
		}

		return true;
	}

	/** Can this item be destroyed/salvaged/sold? */
	bool CanDestroy(const FString& RequestorOwnerId) const
	{
		if (!IsValid()) return false;

		// Heirlooms default to indestructible; allow a future “unbind” system to override.
		if (bNonDestructible) return false;

		// If you want to allow the bound owner to destroy (not typical), change this to:
		// return !bIsHeirloom || IsHeirloomOwnedBy(RequestorOwnerId);
		return !bIsHeirloom;
	}

	/** Can this item be dropped into the world? Typically same as CanDestroy, but separate if your design differs. */
	bool CanDrop(const FString& RequestorOwnerId) const
	{
		// Most games forbid dropping heirlooms. If you allow owner-only dropping, tweak here.
		return !bIsHeirloom && !bNonTransferable;
	}

	/** Ensure invariants for heirlooms: quantity 1 and non-stackable semantics. Call when marking as heirloom. */
	void ApplyHeirloomRules()
	{
		if (bIsHeirloom)
		{
			Quantity = FMath::Max(Quantity, 1);
			bNonTransferable = true;
			bNonDestructible = true;

			if (!UniqueInstanceId.IsValid())
			{
				UniqueInstanceId = FGuid::NewGuid();
			}
		}
	}

	/** Useful for logs/tools. */
	FString GetDebugString() const
	{
		const FString AssetPath = ItemData.IsNull() ? TEXT("<null>") : ItemData.ToSoftObjectPath().ToString();
		const FString HeirloomStr = bIsHeirloom ? FString::Printf(TEXT("Heirloom Owner=%s ID=%s"),
			*OwnerPlayerId, *UniqueInstanceId.ToString()) : TEXT("Regular");
		return FString::Printf(TEXT("[Idx=%d Qty=%d Data=%s %s]"), ItemIndex, Quantity, *AssetPath, *HeirloomStr);
	}
};
