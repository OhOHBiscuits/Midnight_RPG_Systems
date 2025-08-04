#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InventoryHelpers.generated.h"

class UInventoryComponent;
class UItemDataAsset;

/**
 * Blueprint Function Library for Inventory Utilities
 */
UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Get an inventory component from any relevant actor/controller/pawn/playerstate/owner chain.
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor);

	// Find item data asset by tag (brute force, for prototyping; replace with DataTable or AssetManager later)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, const FGameplayTag& ItemID);
};
