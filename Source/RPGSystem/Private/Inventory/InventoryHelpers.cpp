#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "UObject/UObjectIterator.h"

// Universal inventory getter. Works on AActor, APlayerController, APawn, etc.
UInventoryComponent* UInventoryHelpers::GetInventoryComponent(AActor* Actor)
{
    if (!Actor) return nullptr;

    // 1. Actor itself
    if (UInventoryComponent* Comp = Actor->FindComponentByClass<UInventoryComponent>())
        return Comp;

    // 2. PlayerController
    if (APlayerController* PC = Cast<APlayerController>(Actor))
    {
        if (UInventoryComponent* Comp = PC->FindComponentByClass<UInventoryComponent>())
            return Comp;
        if (PC->PlayerState)
            if (UInventoryComponent* Comp = PC->PlayerState->FindComponentByClass<UInventoryComponent>())
                return Comp;
    }

    // 3. Pawn
    if (APawn* Pawn = Cast<APawn>(Actor))
    {
        if (UInventoryComponent* Comp = Pawn->FindComponentByClass<UInventoryComponent>())
            return Comp;
        if (AController* C = Pawn->GetController())
        {
            if (UInventoryComponent* Comp = C->FindComponentByClass<UInventoryComponent>())
                return Comp;
            if (C->PlayerState)
                if (UInventoryComponent* Comp = C->PlayerState->FindComponentByClass<UInventoryComponent>())
                    return Comp;
        }
    }

    // 4. Owner (traverse chain)
    if (AActor* Owner = Actor->GetOwner())
        return GetInventoryComponent(Owner);

    return nullptr;
}

// Brute-force search for UItemDataAsset by GameplayTag.
// Replace with a DataTable or Asset Registry search for production!
UItemDataAsset* UInventoryHelpers::FindItemDataByTag(UObject* WorldContextObject, const FGameplayTag& ItemID)
{
    for (TObjectIterator<UItemDataAsset> It; It; ++It)
    {
        if (It->ItemIDTag == ItemID)
        {
            return *It;
        }
    }
    return nullptr;
}
