#include "Crafting/CraftingHelpers.h"
#include "Inventory/ItemDataAsset.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void UCraftingHelpers::ResolveSoftArray(const TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>>& In,
	TArray<UCraftingRecipeDataAsset*>& Out)
{
	Out.Reset();
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : In)
	{
		if (!Soft.ToSoftObjectPath().IsValid()) continue;

		UCraftingRecipeDataAsset* Resolved = Soft.Get();
		if (!Resolved)
		{
			Resolved = Cast<UCraftingRecipeDataAsset>(Streamable.LoadSynchronous(Soft.ToSoftObjectPath(), false));
		}
		if (Resolved) Out.Add(Resolved);
	}
}

void UCraftingHelpers::GetRecipesForItem(UItemDataAsset* Item,
	TArray<UCraftingRecipeDataAsset*>& OutCraftable,
	TArray<UCraftingRecipeDataAsset*>& OutUsedIn)
{
	OutCraftable.Reset();
	OutUsedIn.Reset();

	if (!Item || !Item->bCraftingEnabled) return;

	ResolveSoftArray(Item->CraftableRecipes, OutCraftable);
	ResolveSoftArray(Item->UsedInRecipes, OutUsedIn);
}

bool UCraftingHelpers::ItemCanBeCrafted(UItemDataAsset* Item)
{
	if (!Item || !Item->bCraftingEnabled) return false;

	TArray<UCraftingRecipeDataAsset*> Craftable, UsedIn;
	GetRecipesForItem(Item, Craftable, UsedIn);
	return Craftable.Num() > 0;
}

void UCraftingHelpers::GetRecipeNamesForItem(UItemDataAsset* Item,
	TArray<FText>& OutCraftableNames,
	TArray<FText>& OutUsedInNames)
{
	OutCraftableNames.Reset();
	OutUsedInNames.Reset();

	TArray<UCraftingRecipeDataAsset*> Craftable, UsedIn;
	GetRecipesForItem(Item, Craftable, UsedIn);

	for (UCraftingRecipeDataAsset* R : Craftable)
	{
		OutCraftableNames.Add(FText::FromName(R ? R->GetFName() : NAME_None));
	}
	for (UCraftingRecipeDataAsset* R : UsedIn)
	{
		OutUsedInNames.Add(FText::FromName(R ? R->GetFName() : NAME_None));
	}
}
