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

	virtual void StartInitialLoading() override;

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Assets")
	UItemDataAsset* LoadItemDataByTag(const FGameplayTag& ItemID, bool bSyncLoad = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="2_Inventory|Assets")
	bool ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath) const;

private:
	TMap<FGameplayTag, FSoftObjectPath> ItemTagToPath;
	void BuildItemIndex();
};
