// WorkstationDataAsset.cpp
#include "Inventory/WorkstationDataAsset.h"
#include "Crafting/CraftingRecipeDataAsset.h"

bool UWorkstationDataAsset::IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe) const
{
	if (!Recipe) return false;

	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : AllowedRecipes)
	{
		if (const UCraftingRecipeDataAsset* Loaded = Soft.Get())
		{
			if (Loaded == Recipe)
			{
				return true;
			}
		}
	}
	return AllowedRecipes.Num() == 0; // if empty, treat as "allow all"
}
