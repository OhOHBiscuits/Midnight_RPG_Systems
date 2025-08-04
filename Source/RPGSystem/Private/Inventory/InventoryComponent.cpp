#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Inventory/ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    Items.SetNum(MaxSlots);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryComponent, Items);
}

// --- Actions ---
bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!ItemData || Quantity <= 0)
        return false;

    int32 StackSlot = FindStackableSlot(ItemData);
    int32 FreeSlot = (StackSlot == INDEX_NONE) ? FindFreeSlot() : StackSlot;
    if (FreeSlot == INDEX_NONE)
        return false;

    // Capacity check
    if (GetCurrentWeight() + ItemData->Weight * Quantity > MaxCarryWeight)
        return false;
    if (GetCurrentVolume() + ItemData->Volume * Quantity > MaxCarryVolume)
        return false;

    if (Items[FreeSlot].IsValid())
    {
        int32 AddQty = FMath::Min(Quantity, ItemData->MaxStackSize - Items[FreeSlot].Quantity);
        Items[FreeSlot].Quantity += AddQty;
        Quantity -= AddQty;
    }
    else
    {
        Items[FreeSlot] = FInventoryItem(ItemData, Quantity, FreeSlot);
    }

    NotifySlotChanged(FreeSlot);
    NotifyInventoryChanged();
    UpdateItemIndexes();
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
        return false;

    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity < Quantity)
        return false;

    Item.Quantity -= Quantity;
    if (Item.Quantity <= 0)
        Items[SlotIndex] = FInventoryItem();

    NotifySlotChanged(SlotIndex);
    NotifyInventoryChanged();
    UpdateItemIndexes();
    return true;
}

bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity)
{
    int32 SlotIndex = FindSlotWithItemID(ItemID);
    return RemoveItem(SlotIndex, Quantity);
}

bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex)
        return false;

    FInventoryItem& From = Items[FromIndex];
    FInventoryItem& To = Items[ToIndex];
    if (!From.IsValid())
        return false;

    if (To.IsValid() && From.ItemData == To.ItemData && From.IsStackable())
    {
        int32 TransferQty = FMath::Min(From.Quantity, To.ItemData.Get()->MaxStackSize - To.Quantity);
        if (TransferQty <= 0) return false;

        To.Quantity += TransferQty;
        From.Quantity -= TransferQty;
        if (From.Quantity <= 0)
            From = FInventoryItem();
    }
    else
    {
        Swap(From, To);
    }

    NotifySlotChanged(FromIndex);
    NotifySlotChanged(ToIndex);
    NotifyInventoryChanged();
    UpdateItemIndexes();
    return true;
}

bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (!TargetInventory || !Items.IsValidIndex(FromIndex))
        return false;

    FInventoryItem& Item = Items[FromIndex];
    if (!Item.IsValid())
        return false;

    UItemDataAsset* ItemData = Item.ItemData.Get();
    int32 Quantity = Item.Quantity;

    bool bAdded = TargetInventory->TryAddItem(ItemData, Quantity);
    if (bAdded)
    {
        Items[FromIndex] = FInventoryItem();
        NotifySlotChanged(FromIndex);
        NotifyInventoryChanged();
        UpdateItemIndexes();
        TargetInventory->NotifyInventoryChanged();
        return true;
    }
    return false;
}

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!GetOwner())
        return false;
    if (GetOwner()->HasAuthority())
    {
        // Only the server does the real logic and returns a real result.
        return AddItem(ItemData, Quantity);
    }
    else
    {
        // Client: Ask the server to do it, but return true to mean "request sent"
        ServerAddItem(ItemData, Quantity);
        return true;
    }
}

bool UInventoryComponent::TryRemoveItem(int32 SlotIndex, int32 Quantity)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
        return RemoveItem(SlotIndex, Quantity);
    ServerRemoveItem(SlotIndex, Quantity);
    return false;
}

bool UInventoryComponent::TryMoveItem(int32 FromIndex, int32 ToIndex)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
        return MoveItem(FromIndex, ToIndex);
    ServerMoveItem(FromIndex, ToIndex);
    return false;
}

bool UInventoryComponent::TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
        return TransferItemToInventory(FromIndex, TargetInventory);
    ServerTransferItem(FromIndex, TargetInventory);
    return false;
}

bool UInventoryComponent::RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
    if (!SourceInventory || !TargetInventory) return false;
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
    {
        // Example: Pull from source, add to target, clear source slot
        FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
        if (!Item.IsValid()) return false;
        if (TargetInventory->TryAddItem(Item.ItemData.Get(), Item.Quantity))
        {
            SourceInventory->RemoveItem(SourceIndex, Item.Quantity);
            return true;
        }
        return false;
    }
    else
    {
        Server_TransferItem(SourceInventory, SourceIndex, TargetInventory, TargetIndex);
        return false;
    }
}

// --- Queries ---
bool UInventoryComponent::IsInventoryFull() const
{
    for (const FInventoryItem& Item : Items)
        if (!Item.IsValid()) return false;
    return true;
}

float UInventoryComponent::GetCurrentWeight() const
{
    float Total = 0.f;
    for (const FInventoryItem& Item : Items)
    {
        if (Item.IsValid())
        {
            UItemDataAsset* Data = Item.ItemData.Get();
            if (Data)
                Total += Data->Weight * Item.Quantity;
        }
    }
    return Total;
}

float UInventoryComponent::GetCurrentVolume() const
{
    float Total = 0.f;
    for (const FInventoryItem& Item : Items)
    {
        if (Item.IsValid())
        {
            UItemDataAsset* Data = Item.ItemData.Get();
            if (Data)
                Total += Data->Volume * Item.Quantity;
        }
    }
    return Total;
}

FInventoryItem UInventoryComponent::GetItem(int32 SlotIndex) const
{
    if (Items.IsValidIndex(SlotIndex))
        return Items[SlotIndex];
    return FInventoryItem();
}

int32 UInventoryComponent::FindFreeSlot() const
{
    for (int32 i = 0; i < Items.Num(); ++i)
        if (!Items[i].IsValid()) return i;
    return INDEX_NONE;
}

int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
    if (!ItemData) return INDEX_NONE;
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (Slot.IsValid() && Slot.ItemData == ItemData && Slot.Quantity < ItemData->MaxStackSize)
            return i;
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        UItemDataAsset* Data = Slot.ItemData.Get();
        if (Data && Data->ItemIDTag == ItemID)
            return i;
    }
    return INDEX_NONE;
}

FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
    int32 SlotIndex = FindSlotWithItemID(ItemID);
    return (SlotIndex != INDEX_NONE) ? Items[SlotIndex] : FInventoryItem();
}

// --- Delegates ---
void UInventoryComponent::NotifySlotChanged(int32 SlotIndex)
{
    OnInventoryUpdated.Broadcast(SlotIndex);
}

void UInventoryComponent::NotifyInventoryChanged()
{
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::UpdateItemIndexes()
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        Items[i].ItemIndex = i;
    }
}

// --- Settings ---
void UInventoryComponent::SetMaxCarryWeight(float NewMaxWeight)
{
    MaxCarryWeight = NewMaxWeight;
}
void UInventoryComponent::SetMaxCarryVolume(float NewMaxVolume)
{
    MaxCarryVolume = NewMaxVolume;
}

// --- Replication ---
void UInventoryComponent::OnRep_InventoryItems()
{
    NotifyInventoryChanged();
}

// --- RPCs ---
void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData, int32 Quantity)
{
    bool bSuccess = AddItem(ItemData, Quantity);
    ClientAddItemResponse(bSuccess);
}
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* ItemData, int32 Quantity) { return true; }

void UInventoryComponent::ClientAddItemResponse_Implementation(bool bSuccess) {}

void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex, int32 Quantity)
{
    RemoveItem(SlotIndex, Quantity);
}
bool UInventoryComponent::ServerRemoveItem_Validate(int32 SlotIndex, int32 Quantity) { return true; }

void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID, int32 Quantity)
{
    RemoveItemByID(ItemID, Quantity);
}
bool UInventoryComponent::ServerRemoveItemByID_Validate(FGameplayTag ItemID, int32 Quantity) { return true; }

void UInventoryComponent::ClientRemoveItemResponse_Implementation(bool bSuccess) {}

void UInventoryComponent::ServerMoveItem_Implementation(int32 FromIndex, int32 ToIndex)
{
    MoveItem(FromIndex, ToIndex);
}
bool UInventoryComponent::ServerMoveItem_Validate(int32 FromIndex, int32 ToIndex) { return true; }

void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    TransferItemToInventory(FromIndex, TargetInventory);
}
bool UInventoryComponent::ServerTransferItem_Validate(int32 FromIndex, UInventoryComponent* TargetInventory) { return true; }

void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex, UInventoryComponent* SourceInventory)
{
    if (!SourceInventory || SourceInventory == this)
        return;

    FInventoryItem FromSlot = SourceInventory->GetItem(FromIndex);
    if (!FromSlot.IsValid())
        return;

    UItemDataAsset* Asset = FromSlot.ItemData.Get();
    int32 Quantity = FromSlot.Quantity;

    if (Asset && TryAddItem(Asset, Quantity))
        SourceInventory->RemoveItem(FromIndex, Quantity);
}
bool UInventoryComponent::ServerPullItem_Validate(int32 FromIndex, UInventoryComponent* SourceInventory) { return true; }

void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
    if (!SourceInventory || !TargetInventory) return;
    FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
    if (!Item.IsValid()) return;

    if (TargetInventory->TryAddItem(Item.ItemData.Get(), Item.Quantity))
        SourceInventory->RemoveItem(SourceIndex, Item.Quantity);
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex) { return true; }

int32 UInventoryComponent::GetNumOccupiedSlots() const
{
    int32 Count = 0;
    for (const FInventoryItem& Item : Items)
        if (Item.IsValid()) ++Count;
    return Count;
}

int32 UInventoryComponent::GetNumItemsOfType(FGameplayTag ItemID) const
{
    int32 Count = 0;
    for (const FInventoryItem& Item : Items)
        if (Item.IsValid() && Item.ItemData.IsValid() && Item.ItemData.Get()->ItemIDTag == ItemID)
            Count += Item.Quantity;
    return Count;
}

bool UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB)
{
    if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB)
        return false;
    Swap(Items[IndexA], Items[IndexB]);
    UpdateItemIndexes();
    NotifySlotChanged(IndexA);
    NotifySlotChanged(IndexB);
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::CanAcceptItem(UItemDataAsset* ItemData) const
{
    if (!ItemData) return false;
    if (AllowedItemIDs.Num() == 0)
        return true; // No restriction
    return AllowedItemIDs.Contains(ItemData->ItemIDTag);
}

int32 UInventoryComponent::GetNumUISlots() const
{
    return Items.Num();
}

void UInventoryComponent::GetUISlotInfo(
    TArray<int32>& OutSlotIndices,
    TArray<UItemDataAsset*>& OutItemData,
    TArray<int32>& OutQuantities) const
{
    OutSlotIndices.Empty();
    OutItemData.Empty();
    OutQuantities.Empty();

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        OutSlotIndices.Add(i);
        OutItemData.Add(Items[i].ItemData.Get());   // Use .Get() for soft object pointer
        OutQuantities.Add(Items[i].Quantity);
    }
}

