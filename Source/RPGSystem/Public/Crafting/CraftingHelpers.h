#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingHelpers.generated.h"

class UItemDataAsset;
class UCraftingRecipeDataAsset;

UCLASS()
class RPGSYSTEM_API UCraftingHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting|UI")
	static void GetRecipesForItem(UItemDataAsset* Item,
		TArray<UCraftingRecipeDataAsset*>& OutCraftable,
		TArray<UCraftingRecipeDataAsset*>& OutUsedIn);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting|UI")
	static bool ItemCanBeCrafted(UItemDataAsset* Item);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Crafting|UI")
	static void GetRecipeNamesForItem(UItemDataAsset* Item,
		TArray<FText>& OutCraftableNames,
		TArray<FText>& OutUsedInNames);

private:
	static void ResolveSoftArray(const TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>>& In,
		TArray<UCraftingRecipeDataAsset*>& Out);
};
