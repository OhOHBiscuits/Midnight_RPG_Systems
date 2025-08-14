// InventoryAssetManager.cpp
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"

UInventoryAssetManager& UInventoryAssetManager::Get()
{
	
	return *CastChecked<UInventoryAssetManager>(&UAssetManager::Get());
}
UInventoryAssetManager* UInventoryAssetManager::GetOptional()
{
	if (UAssetManager* AM = UAssetManager::GetIfInitialized())
	{
		return Cast<UInventoryAssetManager>(AM); // may be nullptr if not configured
	}
	return nullptr;
}

void UInventoryAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	BuildItemIndex();
}

void UInventoryAssetManager::BuildItemIndex()
{
	ItemTagToPath.Empty();

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AR = ARM.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UItemDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AR.GetAssets(Filter, Assets);

	static const FName ItemIDTagName(TEXT("ItemIDTag"));

	for (const FAssetData& AD : Assets)
	{
		FString TagString;
		if (AD.GetTagValue(ItemIDTagName, TagString) && !TagString.IsEmpty())
		{
			const FGameplayTag ItemTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
			if (ItemTag.IsValid())
			{
				ItemTagToPath.Add(ItemTag, AD.ToSoftObjectPath());
			}
		}
	}
}

bool UInventoryAssetManager::ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath) const
{
	if (!ItemID.IsValid()) return false;
	if (const FSoftObjectPath* Found = ItemTagToPath.Find(ItemID))
	{
		OutPath = *Found;
		return OutPath.IsValid();
	}
	return false;
}

UItemDataAsset* UInventoryAssetManager::LoadItemDataByTag(const FGameplayTag& ItemID, bool bSyncLoad)
{
	if (!ItemID.IsValid()) return nullptr;

	FSoftObjectPath Path;
	if (!ResolveItemPathByTag(ItemID, Path)) return nullptr;

	FStreamableManager& SM = UAssetManager::GetStreamableManager();
	if (bSyncLoad)
	{
		return Cast<UItemDataAsset>(SM.LoadSynchronous(Path, false));
	}

	TSharedPtr<FStreamableHandle> Handle = SM.RequestAsyncLoad(Path, FStreamableDelegate());
	if (Handle.IsValid() && Handle->HasLoadCompleted())
	{
		return Cast<UItemDataAsset>(Path.ResolveObject());
	}
	return nullptr;
}

UItemDataAsset* UInventoryAssetManager::FindItemDataByTag(const FGameplayTag& ItemIdTag) const
{
	if (!ItemIdTag.IsValid())
	{
		return nullptr;
	}

	// Your existing lookup code here (PrimaryAssetId/Registry search, etc.)
	// Placeholder example using Asset Registry tags (adjust to your implementation):
	// return <... found UItemDataAsset* by ItemIDTag ...>;

	return nullptr; // leave as-is if you already implemented it elsewhere
}