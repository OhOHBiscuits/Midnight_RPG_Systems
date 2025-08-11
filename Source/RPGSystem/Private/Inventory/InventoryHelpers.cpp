#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "UObject/UObjectIterator.h"

UInventoryComponent* UInventoryHelpers::GetInventoryComponent(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (UInventoryComponent* Comp = Actor->FindComponentByClass<UInventoryComponent>()) return Comp;

	if (APlayerController* PC = Cast<APlayerController>(Actor))
	{
		if (UInventoryComponent* C = PC->FindComponentByClass<UInventoryComponent>()) return C;
		if (PC->PlayerState)
			if (UInventoryComponent* C2 = PC->PlayerState->FindComponentByClass<UInventoryComponent>()) return C2;
	}

	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (UInventoryComponent* C = Pawn->FindComponentByClass<UInventoryComponent>()) return C;
		if (AController* Cntrl = Pawn->GetController())
		{
			if (UInventoryComponent* C2 = Cntrl->FindComponentByClass<UInventoryComponent>()) return C2;
			if (Cntrl->PlayerState)
				if (UInventoryComponent* C3 = Cntrl->PlayerState->FindComponentByClass<UInventoryComponent>()) return C3;
		}
	}

	if (AActor* Owner = Actor->GetOwner())
		return GetInventoryComponent(Owner);

	return nullptr;
}

UItemDataAsset* UInventoryHelpers::FindItemDataByTag(UObject* /*WorldContextObject*/, const FGameplayTag& ItemID)
{
#if WITH_EDITOR
	// Editor/PIE path still works for already-loaded assets (nice in PIE)
	for (TObjectIterator<UItemDataAsset> It; It; ++It)
	{
		if (It->ItemIDTag == ItemID)
			return *It;
	}
#endif
	// Cooked/Standalone
	return UInventoryAssetManager::Get().LoadItemDataByTag(ItemID, /*bSyncLoad=*/true);
}

bool UInventoryHelpers::ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath)
{
	return UInventoryAssetManager::Get().ResolveItemPathByTag(ItemID, OutPath);
}
