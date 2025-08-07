#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

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
// --- Limit helpers ---
bool UInventoryComponent::IsVolumeLimited() const
{
    return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Volume")));
}
bool UInventoryComponent::IsWeightLimited() const
{
    return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Weight")));
}
bool UInventoryComponent::IsSlotLimited() const
{
    return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Slot")));
}
bool UInventoryComponent::IsUnlimited() const
{
    return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.None")));
}
// --- Actions ---
bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!ItemData || Quantity <= 0)
        return false;

    int32 StackSlot = FindStackableSlot(ItemData);
    int32 FreeSlot = (StackSlot == INDEX_NONE) ? FindFreeSlot() : StackSlot;
    if (FreeSlot == INDEX_NONE)
    {
        OnInventoryFull.Broadcast(true);
        return false;
    }

    // --- Unlimited: always allow add ---
    if (IsUnlimited())
    {
        if (Items[FreeSlot].IsValid())
        {
            Items[FreeSlot].Quantity += Quantity;
        }
        else
        {
            Items[FreeSlot] = FInventoryItem(ItemData, Quantity, FreeSlot);
        }
        OnItemAdded.Broadcast(Items[FreeSlot], Quantity);

        if (bAutoSort)
        {
            RequestSortInventoryByName();
        }
        NotifySlotChanged(FreeSlot);        
        NotifyInventoryChanged();        
        UpdateItemIndexes();
        OnInventoryFull.Broadcast(false);
        return true;
    }

    // --- Volume/Weight-limited: check respective limit, ignore max stack size
    if (IsVolumeLimited())
    {
        float NewVolume = GetCurrentVolume() + (ItemData->Volume * Quantity);
        if (NewVolume > MaxCarryVolume)
        {
            OnInventoryFull.Broadcast(true);
            return false;
        }
    }
    if (IsWeightLimited())
    {
        float NewWeight = GetCurrentWeight() + (ItemData->Weight * Quantity);
        if (NewWeight > MaxCarryWeight)
        {
            OnInventoryFull.Broadcast(true);
            return false;
        }
    }

    // --- Slot-limited (default RPG style): stack up to max stack
    int32 AddQty = Quantity;
    if (IsSlotLimited() && Items[FreeSlot].IsValid())
    {
        AddQty = FMath::Min(Quantity, ItemData->MaxStackSize - Items[FreeSlot].Quantity);
        if (AddQty <= 0)
        {
            OnInventoryFull.Broadcast(true);
            return false;
        }
    }
    if (Items[FreeSlot].IsValid())
    {
        Items[FreeSlot].Quantity += AddQty;
        Quantity -= AddQty;
    }
    else
    {
        Items[FreeSlot] = FInventoryItem(ItemData, AddQty, FreeSlot);
        Quantity -= AddQty;
    }

    OnItemAdded.Broadcast(Items[FreeSlot], AddQty);
    if (bAutoSort)
    {
        RequestSortInventoryByName();
    }
    NotifySlotChanged(FreeSlot);
    NotifyInventoryChanged();
    UpdateItemIndexes();

    if (IsInventoryFull())
        OnInventoryFull.Broadcast(true);
    else
        OnInventoryFull.Broadcast(false);
  

    return true;
}
bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
        return false;

    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity < Quantity)
        return false;

    OnItemRemoved.Broadcast(Item);
    Item.Quantity -= Quantity;
    if (Item.Quantity <= 0)
        Items[SlotIndex] = FInventoryItem();
    
   if (bAutoSort)
       {
           RequestSortInventoryByName();
       }
    //Dispatchers
    
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
        OnItemTransferSuccess.Broadcast(Item);
        
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
        return AddItem(ItemData, Quantity);
    }
    else
    {
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
        FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
        if (!Item.IsValid()) return false;
        if (TargetInventory->TryAddItem(Item.ItemData.Get(), Item.Quantity))
        {
            SourceInventory->RemoveItem(SourceIndex, Item.Quantity);
            OnItemTransferSuccess.Broadcast(Item);
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
    if (IsUnlimited()) return false;
    if (IsSlotLimited())
        return FindFreeSlot() == INDEX_NONE;
    if (IsWeightLimited())
        return GetCurrentWeight() >= MaxCarryWeight;
    if (IsVolumeLimited())
        return GetCurrentVolume() >= MaxCarryVolume;
    return false;
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
        if (Slot.IsValid() && Slot.ItemData == ItemData && (IsSlotLimited() ? Slot.Quantity < ItemData->MaxStackSize : true))
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
// --- UI Helper ---
int32 UInventoryComponent::GetNumUISlots() const
{
    return MaxSlots;
}
void UInventoryComponent::GetUISlotInfo(TArray<int32>& OutSlotIndices, TArray<UItemDataAsset*>& OutItemData, TArray<int32>& OutQuantities) const
{
    OutSlotIndices.Empty();
    OutItemData.Empty();
    OutQuantities.Empty();
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        OutSlotIndices.Add(i);
        OutItemData.Add(Items[i].ItemData.Get());
        OutQuantities.Add(Items[i].Quantity);
    }
}
void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots)
{
    MaxSlots = FMath::Clamp(NewMaxSlots, 1, 1000); // prevent weird values
    AdjustSlotCountIfNeeded();
    NotifyInventoryChanged();
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
void UInventoryComponent::AdjustSlotCountIfNeeded()
{
    int32 TargetSlots = MaxSlots;

    // For Volume/Weight/Unlimited, allow overflow by 2 if needed
    if (!IsSlotLimited())
    {
        if (Items.Num() > TargetSlots)
        {
            TargetSlots = Items.Num() + 2;
        }
        else
        {
            TargetSlots += 2; // always give a couple of extra empty slots for UI
        }
    }
    // Slot-limited: do not expand
    else
    {
        TargetSlots = MaxSlots;
    }

    // Resize (preserves data up to new length, pads with empty if needed)
    Items.SetNum(TargetSlots);
    UpdateItemIndexes();
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
void UInventoryComponent::ServerSortInventoryByName_Implementation()
{
    SortInventoryByName();
}
bool UInventoryComponent::ServerSortInventoryByName_Validate()
{
    return true;
}
void UInventoryComponent::RequestSortInventoryByName()
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
    {
        SortInventoryByName();
    }
    else
    {
        ServerSortInventoryByName();
    }
}
void UInventoryComponent::SortInventoryByName()
{
    // Sort by item name, treating null/empty as last
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();
        FString NameA = (DA ? DA->Name.ToString() : FString());
        FString NameB = (DB ? DB->Name.ToString() : FString());
        // Place empty items at the end
        if (!A.IsValid()) return false;
        if (!B.IsValid()) return true;
        return NameA < NameB;
    });

    
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
// --- Sort by Rarity ---
void UInventoryComponent::SortInventoryByRarity()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        UItemDataAsset* AData = A.ItemData.Get();
        UItemDataAsset* BData = B.ItemData.Get();
        if (!AData || !BData) return false;
        // Lower "Rarity" tag sorts first (customize as needed)
        return AData->Rarity.ToString() < BData->Rarity.ToString();
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::RequestSortInventoryByRarity()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SortInventoryByRarity();
    }
    else
    {
        ServerSortInventoryByRarity();
    }
}
void UInventoryComponent::ServerSortInventoryByRarity_Implementation()
{
    SortInventoryByRarity();
}
bool UInventoryComponent::ServerSortInventoryByRarity_Validate() { return true; }
// --- Sort by Type ---
void UInventoryComponent::SortInventoryByType()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        UItemDataAsset* AData = A.ItemData.Get();
        UItemDataAsset* BData = B.ItemData.Get();
        if (!AData || !BData) return false;
        return AData->ItemType.ToString() < BData->ItemType.ToString();
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::RequestSortInventoryByType()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SortInventoryByType();
    }
    else
    {
        ServerSortInventoryByType();
    }
}
void UInventoryComponent::ServerSortInventoryByType_Implementation()
{
    SortInventoryByType();
}
bool UInventoryComponent::ServerSortInventoryByType_Validate() { return true; }
// --- Sort by Category ---
void UInventoryComponent::SortInventoryByCategory()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        UItemDataAsset* AData = A.ItemData.Get();
        UItemDataAsset* BData = B.ItemData.Get();
        if (!AData || !BData) return false;
        return AData->ItemCategory.ToString() < BData->ItemCategory.ToString();
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::RequestSortInventoryByCategory()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SortInventoryByCategory();
    }
    else
    {
        ServerSortInventoryByCategory();
    }
}
void UInventoryComponent::ServerSortInventoryByCategory_Implementation()
{
    SortInventoryByCategory();
}
bool UInventoryComponent::ServerSortInventoryByCategory_Validate() { return true; }
TArray<FInventoryItem> UInventoryComponent::FilterItemsByRarity(FGameplayTag RarityTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->Rarity == RarityTag)
            Results.Add(Item);
    }
    return Results;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByCategory(FGameplayTag CategoryTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemCategory == CategoryTag)
            Results.Add(Item);
    }
    return Results;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemSubCategory == SubCategoryTag)
            Results.Add(Item);
    }
    return Results;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByType(FGameplayTag TypeTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemType == TypeTag)
            Results.Add(Item);
    }
    return Results;
}
// Filter by multiple tags (AND or OR)
TArray<FInventoryItem> UInventoryComponent::FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        UItemDataAsset* Data = Item.ItemData.Get();
        if (!Data) continue;

        FGameplayTagContainer ItemTags;
        ItemTags.AddTag(Data->Rarity);
        ItemTags.AddTag(Data->ItemType);
        ItemTags.AddTag(Data->ItemCategory);
        ItemTags.AddTag(Data->ItemSubCategory);

        if (bMatchAll)
        {
            if (ItemTags.HasAll(Tags))
                Results.Add(Item);
        }
        else
        {
            if (ItemTags.HasAny(Tags))
                Results.Add(Item);
        }
    }
    return Results;
}
bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity)
{
    if (!Items.IsValidIndex(SlotIndex) || SplitQuantity <= 0)
        return false;
    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity <= SplitQuantity)
        return false; // Can't split more than we have, or leave zero

    int32 FreeSlot = FindFreeSlot();
    if (FreeSlot == INDEX_NONE)
        return false; // No free slot

    // Reduce original, create new stack
    Item.Quantity -= SplitQuantity;
    Items[FreeSlot] = FInventoryItem(Item.ItemData.Get(), SplitQuantity, FreeSlot);
    if (bAutoSort)
    {
        RequestSortInventoryByName();
    }
    NotifySlotChanged(SlotIndex);
    NotifySlotChanged(FreeSlot);
    NotifyInventoryChanged();
    UpdateItemIndexes();
    return true;
}
void UInventoryComponent::ServerSplitStack_Implementation(int32 SlotIndex, int32 SplitQuantity)
{
    SplitStack(SlotIndex, SplitQuantity);
}
bool UInventoryComponent::ServerSplitStack_Validate(int32 SlotIndex, int32 SplitQuantity) { return true; }
bool UInventoryComponent::TrySplitStack(int32 SlotIndex, int32 SplitQuantity)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
        return SplitStack(SlotIndex, SplitQuantity);
    ServerSplitStack(SlotIndex, SplitQuantity);
    return false;
}



