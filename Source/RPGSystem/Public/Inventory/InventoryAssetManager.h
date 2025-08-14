// InventoryAssetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GameplayTagContainer.h"
#include "InventoryAssetManager.generated.h"

class UItemDataAsset;

UCLASS()
class RPGSYSTEM_API UInventoryAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UInventoryAssetManager& Get();

	static UInventoryAssetManager* GetOptional();

	virtual void StartInitialLoading() override;

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Assets")
	UItemDataAsset* LoadItemDataByTag(const FGameplayTag& ItemID, bool bSyncLoad = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Assets")
	bool ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath) const;
	

	// Make this PUBLIC so helpers/other systems can use it.
	UItemDataAsset* FindItemDataByTag(const FGameplayTag& ItemIdTag) const;
private:
	TMap<FGameplayTag, FSoftObjectPath> ItemTagToPath;
	void BuildItemIndex();

	
};
