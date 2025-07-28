#include "Inventory/InventoryItem.h"
#include "Inventory/ItemDataAsset.h"

bool FInventoryItem::IsStackable() const
{
	return ItemData.IsValid() && ItemData->MaxStackSize > 1;
}
