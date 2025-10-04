// InventoryAssetManager.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GameplayTagContainer.h"
#include "InventoryAssetManager.generated.h"

class UItemDataAsset;
class UDataAsset;

/**
 * Generic tag->asset lookup for ANY DataAsset class.
 * Uses Asset Registry searchable tags when possible (no load),
 * and soft-loads only as a fallback. Works in PIE and cooked Standalone.
 */
UCLASS()
class RPGSYSTEM_API UInventoryAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// --- Boilerplate ---
	static UInventoryAssetManager& Get();
	static UInventoryAssetManager* GetOptional();

	virtual void StartInitialLoading() override;

	// ===========================
	// Legacy (kept) - Items only
	// ===========================
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Assets")
	UItemDataAsset* LoadItemDataByTag(const FGameplayTag& ItemID, bool bSyncLoad = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Assets")
	bool ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath) const;

	// Direct C++ helper (already-loaded)
	UItemDataAsset* FindItemDataByTag(const FGameplayTag& ItemIdTag) const;

	// =======================================================
	// NEW: Generic tagged loading for ANY UDataAsset class
	// =======================================================

	/** Register a DataAsset class that has a "single ID" FGameplayTag property.
	 *  @param AssetClass            The DataAsset subclass to index (e.g., UItemDataAsset::StaticClass()).
	 *  @param TagPropertyName       The UPROPERTY name of the ID tag field (e.g., "ItemIDTag", "RecipeIDTag").
	 *  @param AssetRegistryKeyName  Optional override for the AssetRegistry key; defaults to TagPropertyName.
	 */
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Assets")
	void RegisterTaggedClass(TSubclassOf<UDataAsset> AssetClass, FName TagPropertyName, FName AssetRegistryKeyName = NAME_None);

	/** Returns a soft path to the asset with the given ID tag for the requested class. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Assets")
	bool ResolveDataAssetPathByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, FSoftObjectPath& OutPath) const;

	/** Loads (sync) or requests (async) the asset. For async, returns nullptr immediately; prefer sync for server logic. */
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Assets")
	UDataAsset* LoadDataAssetByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, bool bSyncLoad = true);

	/** Convenience: templated C++ load (strongly-typed). */
	template<typename TAsset>
	TAsset* LoadDataAssetByTag(const FGameplayTag& Tag, bool bSyncLoad = true)
	{
		return Cast<TAsset>(LoadDataAssetByTag(Tag, TAsset::StaticClass(), bSyncLoad));
	}

protected:
	// Build the legacy item index (kept) and the generic class maps.
	void BuildItemIndex();
	void BuildGenericTagIndices();

	// Internal: try to index a class using AssetRegistry tags; fallback to soft load to read property if needed.
	void IndexTaggedClass(UClass* InClass, FName TagPropName, FName AssetRegistryKeyName);

	// Internal: reflection reader for FGameplayTag property on a loaded object.
	static bool TryReadGameplayTagProperty(UObject* Obj, FName TagPropName, FGameplayTag& OutTag);

private:
	// --------- Legacy (items) ----------
	UPROPERTY() // OK: simple map, no nested maps/weak keys
	TMap<FGameplayTag, FSoftObjectPath> ItemTagToPath;

	// --------- Generic maps (runtime only; not reflected) ------------
	struct FTaggedClassCfg
	{
		UClass* AssetClass = nullptr; // UClass is never GC'd; no need for weak ptr
		FName   TagProp;              // e.g., "ItemIDTag", "RecipeIDTag"
		FName   RegistryKey;          // usually same as TagProp; used for AssetRegistry lookup
	};

	// No UPROPERTY: avoids UHT constraints (weak keys, nested maps).
	TArray<FTaggedClassCfg> TaggedClassConfigs;

	// Per-class tag->path map
	TMap<UClass*, TMap<FGameplayTag, FSoftObjectPath>> ClassToTagMap;
};
