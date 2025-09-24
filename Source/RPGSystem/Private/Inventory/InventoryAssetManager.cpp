// InventoryAssetManager.cpp

#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"

#include "Engine/StreamableManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

// Optional headers for concrete asset classes we want to auto-register
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Progression/XPGrantBundle.h"

#define LOCTEXT_NAMESPACE "UInventoryAssetManager"

static IAssetRegistry& GetAR()
{
	// Safe in PIE and cooked
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	return ARM.Get();
}

UInventoryAssetManager& UInventoryAssetManager::Get()
{
	UInventoryAssetManager* Singleton = Cast<UInventoryAssetManager>(GEngine->AssetManager);
	check(Singleton);
	return *Singleton;
}

UInventoryAssetManager* UInventoryAssetManager::GetOptional()
{
	return Cast<UInventoryAssetManager>(GEngine->AssetManager);
}

void UInventoryAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// Legacy: build item map (kept for compatibility with existing calls)
	BuildItemIndex();

	// Register common classes once (you can also do this in DefaultGame.ini via a startup BP call if you prefer)
	RegisterTaggedClass(UItemDataAsset::StaticClass(),           TEXT("ItemIDTag"));     // ensure meta=(AssetRegistrySearchable) on property
	RegisterTaggedClass(UCraftingRecipeDataAsset::StaticClass(), TEXT("RecipeIDTag"));   // ensure meta=(AssetRegistrySearchable)
	RegisterTaggedClass(UXPGrantBundle::StaticClass(),           TEXT("BundleIDTag"));   // ensure meta=(AssetRegistrySearchable)

	// Build generic tag indices
	BuildGenericTagIndices();
}

//
// -------- Legacy items-only API (kept) --------
//
void UInventoryAssetManager::BuildItemIndex()
{
	ItemTagToPath.Empty();

	IAssetRegistry& AR = GetAR();

	// Find all UItemDataAsset assets
	TArray<FAssetData> Assets;
	AR.GetAssetsByClass(UItemDataAsset::StaticClass()->GetClassPathName(), Assets, true);

	for (const FAssetData& AD : Assets)
	{
		FString TagStr;
		// Prefer AssetRegistry tag (fast, no load)
		if (AD.GetTagValue(TEXT("ItemIDTag"), TagStr) && !TagStr.IsEmpty())
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
			if (Tag.IsValid())
			{
				ItemTagToPath.Add(Tag, AD.ToSoftObjectPath());
			}
			continue;
		}

		// Fallback: soft-load just this asset to read property (should be rare)
		if (UObject* Obj = AD.ToSoftObjectPath().TryLoad())
		{
			if (const UItemDataAsset* DA = Cast<UItemDataAsset>(Obj))
			{
				if (DA->ItemIDTag.IsValid())
				{
					ItemTagToPath.Add(DA->ItemIDTag, AD.ToSoftObjectPath());
				}
			}
		}
	}
}

UItemDataAsset* UInventoryAssetManager::LoadItemDataByTag(const FGameplayTag& ItemID, bool bSyncLoad)
{
	FSoftObjectPath Path;
	if (!ResolveItemPathByTag(ItemID, Path))
	{
		return nullptr;
	}

	if (bSyncLoad)
	{
		return Cast<UItemDataAsset>(Path.TryLoad());
	}

	// Async path: queue a load; returns nullptr immediately.
	GetStreamableManager().RequestAsyncLoad(Path, nullptr);
	return nullptr;
}

bool UInventoryAssetManager::ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath) const
{
	if (const FSoftObjectPath* Found = ItemTagToPath.Find(ItemID))
	{
		OutPath = *Found;
		return true;
	}
	return false;
}

UItemDataAsset* UInventoryAssetManager::FindItemDataByTag(const FGameplayTag& ItemIdTag) const
{
	FSoftObjectPath Path;
	return ResolveItemPathByTag(ItemIdTag, Path) ? Cast<UItemDataAsset>(Path.ResolveObject()) : nullptr;
}

//
// -------- NEW: Generic tagged asset API --------
//
void UInventoryAssetManager::RegisterTaggedClass(TSubclassOf<UDataAsset> AssetClass, FName TagPropertyName, FName AssetRegistryKeyName)
{
	if (!AssetClass)
	{
		return;
	}

	FTaggedClassCfg Cfg;
	Cfg.AssetClass = AssetClass.Get();
	Cfg.TagProp    = TagPropertyName;
	Cfg.RegistryKey= (AssetRegistryKeyName == NAME_None) ? TagPropertyName : AssetRegistryKeyName;

	TaggedClassConfigs.Add(Cfg);
}

void UInventoryAssetManager::BuildGenericTagIndices()
{
	ClassToTagMap.Empty();

	for (const FTaggedClassCfg& Cfg : TaggedClassConfigs)
	{
		if (UClass* Cls = Cfg.AssetClass)
		{
			IndexTaggedClass(Cls, Cfg.TagProp, Cfg.RegistryKey);
		}
	}
}

void UInventoryAssetManager::IndexTaggedClass(UClass* InClass, FName TagPropName, FName AssetRegistryKeyName)
{
	if (!InClass)
	{
		return;
	}

	IAssetRegistry& AR = GetAR();

	TArray<FAssetData> Assets;
	AR.GetAssetsByClass(InClass->GetClassPathName(), Assets, true);

	TMap<FGameplayTag, FSoftObjectPath>& Map = ClassToTagMap.FindOrAdd(InClass);

	for (const FAssetData& AD : Assets)
	{
		FGameplayTag FoundTag;

		// 1) Fast path: grab searchable value from AssetRegistry (requires meta=(AssetRegistrySearchable) or GetAssetRegistryTags override)
		FString TagStr;
		if (AD.GetTagValue(AssetRegistryKeyName, TagStr) && !TagStr.IsEmpty())
		{
			FoundTag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
		}

		// 2) Slow path (rare): property wasn't exported to registry — soft-load the asset and read it via reflection.
		if (!FoundTag.IsValid())
		{
			if (UObject* Obj = AD.ToSoftObjectPath().TryLoad())
			{
				if (TryReadGameplayTagProperty(Obj, TagPropName, FoundTag) && FoundTag.IsValid())
				{
					// ok
				}
			}
		}

		if (FoundTag.IsValid())
		{
			Map.FindOrAdd(FoundTag) = AD.ToSoftObjectPath();
		}
	}
}

bool UInventoryAssetManager::TryReadGameplayTagProperty(UObject* Obj, FName TagPropName, FGameplayTag& OutTag)
{
	if (!Obj) return false;

	FProperty* Prop = Obj->GetClass()->FindPropertyByName(TagPropName);
	if (!Prop) return false;

	FStructProperty* StructProp = CastField<FStructProperty>(Prop);
	if (!StructProp || StructProp->Struct != TBaseStructure<FGameplayTag>::Get())
	{
		return false;
	}

	FGameplayTag* TagPtr = StructProp->ContainerPtrToValuePtr<FGameplayTag>(Obj);
	if (!TagPtr) return false;

	OutTag = *TagPtr;
	return true;
}

bool UInventoryAssetManager::ResolveDataAssetPathByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, FSoftObjectPath& OutPath) const
{
	if (!AssetClass || !Tag.IsValid())
	{
		return false;
	}

	const TMap<FGameplayTag, FSoftObjectPath>* Map = ClassToTagMap.Find(AssetClass.Get());
	if (!Map) return false;

	if (const FSoftObjectPath* Found = Map->Find(Tag))
	{
		OutPath = *Found;
		return true;
	}

	return false;
}

UDataAsset* UInventoryAssetManager::LoadDataAssetByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, bool bSyncLoad)
{
	FSoftObjectPath Path;
	if (!ResolveDataAssetPathByTag(Tag, AssetClass, Path))
	{
		return nullptr;
	}

	if (bSyncLoad)
	{
		return Cast<UDataAsset>(Path.TryLoad());
	}

	GetStreamableManager().RequestAsyncLoad(Path, nullptr);
	return nullptr;
}


