#include "Actors/WorkstationActor.h"
#include "Crafting/CraftingStationComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Inventory/WorkstationDataAsset.h"

#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

AWorkstationActor::AWorkstationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CraftingStation = CreateDefaultSubobject<UCraftingStationComponent>(TEXT("CraftingStation"));
	OutputInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("OutputInventory"));
	// Both are UActorComponent; do not call SetupAttachment on them.
}

void AWorkstationActor::BeginPlay()
{
	Super::BeginPlay();

	if (CraftingStation)
	{
		CraftingStation->OutputInventory = OutputInventory;
	}

	// Optionally resolve here so UI has immediate access
	ResolveRecipesSync();
}

void AWorkstationActor::Interact_Implementation(AActor* Interactor)
{
	// Keep your existing flow in BaseWorldItemActor (opens UI, etc.)
	Super::Interact_Implementation(Interactor);

	// If you want to use the data-driven UI class:
	// if (StationConfig && StationConfig->StationWidgetClass.IsValid())
	//   create/show that widget for the Interactor.
}

void AWorkstationActor::ResolveRecipesSync()
{
	if (!StationConfig) return;

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : StationConfig->AllowedRecipes)
	{
		if (Soft.IsNull()) continue;

		UCraftingRecipeDataAsset* R = Soft.LoadSynchronous();
		if (R)
		{
			CachedRecipes.AddUnique(R);
		}
	}
}

const TArray<UCraftingRecipeDataAsset*>& AWorkstationActor::GetAvailableRecipes()
{
	// If someone added/changed the list at runtime, you can re-resolve here.
	if (CachedRecipes.Num() == 0)
	{
		ResolveRecipesSync();
	}
	return reinterpret_cast<const TArray<UCraftingRecipeDataAsset*>&>(CachedRecipes);
}

bool AWorkstationActor::IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe || !StationConfig) return false;

	// Quick path: already cached
	for (const UCraftingRecipeDataAsset* R : CachedRecipes)
	{
		if (R == Recipe) return true;
	}

	// Fallback: see if the soft list contains it (and load if needed)
	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : StationConfig->AllowedRecipes)
	{
		if (Soft.IsNull()) continue;

		// If the soft already points to the same object, approve
		if (Soft.Get() == Recipe)
		{
			CachedRecipes.AddUnique(const_cast<UCraftingRecipeDataAsset*>(Recipe));
			return true;
		}

		// If not loaded, check by path
		if (!Soft.IsValid() && Soft.ToSoftObjectPath() == Recipe->GetPathName())
		{
			CachedRecipes.AddUnique(const_cast<UCraftingRecipeDataAsset*>(Recipe));
			return true;
		}
	}

	return false;
}
