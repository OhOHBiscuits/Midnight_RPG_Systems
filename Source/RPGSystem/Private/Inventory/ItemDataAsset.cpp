#include "Inventory/ItemDataAsset.h"

UItemDataAsset::UItemDataAsset()
	: Super()
{
	// Don’t call RequestGameplayTag here!
	// Tags should be assigned by the designer in the editor,
	// or requested lazily in a function when you actually need them.
}
