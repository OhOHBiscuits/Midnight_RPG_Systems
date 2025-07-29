#include "Inventory/ItemDataAsset.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"


void LoadMeshAsync(UItemDataAsset* ItemData, TFunction<void(UStaticMesh*)> OnLoaded)
{
	if (!ItemData || !ItemData->WorldMesh.IsValid() && !ItemData->WorldMesh.ToSoftObjectPath().IsValid())
	{
		OnLoaded(nullptr);
		return;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	const FSoftObjectPath& MeshPath = ItemData->WorldMesh.ToSoftObjectPath();

	// If already loaded, callback immediately
	if (ItemData->WorldMesh.IsValid())
	{
		OnLoaded(ItemData->WorldMesh.Get());
		return;
	}

	// Otherwise, async load
	Streamable.RequestAsyncLoad(MeshPath, [ItemData, OnLoaded]()
	{
		UStaticMesh* Mesh = ItemData->WorldMesh.Get();
		OnLoaded(Mesh);
	});
}
