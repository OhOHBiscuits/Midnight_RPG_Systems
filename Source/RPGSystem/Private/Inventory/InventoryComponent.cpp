#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

namespace
{
    /** Small helper to protect RPC entry points */
    static bool HasValidOwnerAuthority(const UActorComponent* C)
    {
        const AActor* Owner = C ? C->GetOwner() : nullptr;
        return Owner && Owner->HasAuthority();
    }

    static bool IsClientRequestingOnOwningActor(const UActorComponent* C)
    {
        const AActor* Owner = C ? C->GetOwner() : nullptr;
        return Owner && Owner->GetLocalRole() < ROLE_Authority;
    }
}

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

    if (!CanAcceptItem(ItemData))
        return false;

    const float PrevWeight = GetCurrentWeight();
    const float PrevVolume = GetCurrentVolume();

    int32 StackSlot = FindStackableSlot(ItemData);
    int32 FreeSlot = (StackSlot == INDEX_NONE) ? FindFreeSlot() : StackSlot;
    if (FreeSlot == INDEX_NONE)
    {
        OnInventoryFull.Broadcast(true);
        return false;
    }

    // Unlimited
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
        BroadcastWeightAndVolumeIfChanged(PrevWeight, PrevVolume);
        return true;
    }

    // Volume/Weight limits
    if (IsVolumeLimited())
    {
        const float NewVolume = PrevVolume + (ItemData->Volume * Quantity);
        if (NewVolume > MaxCarryVolume)
        {
            OnInventoryFull.Broadcast(true);
            return false;
        }
    }
    if (IsWeightLimited())
    {
        const float NewWeight = PrevWeight + (ItemData->Weight * Quantity);
        if (NewWeight > MaxCarryWeight)
        {
            OnInventoryFull.Broadcast(true);
            return false;
        }
    }

    // Slot-limited: respect MaxStackSize for existing stacks
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

    OnInventoryFull.Broadcast(IsInventoryFull());
    BroadcastWeightAndVolumeIfChanged(PrevWeight, PrevVolume);
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
        return false;

    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity < Quantity)
        return false;

    const float PrevWeight = GetCurrentWeight();
    const float PrevVolume = GetCurrentVolume();

    OnItemRemoved.Broadcast(Item);

    Item.Quantity -= Quantity;
    if (Item.Quantity <= 0)
    {
        Items[SlotIndex] = FInventoryItem();
    }

    if (bAutoSort)
    {
        RequestSortInventoryByName();
    }

    NotifySlotChanged(SlotIndex);
    NotifyInventoryChanged();
    UpdateItemIndexes();

    BroadcastWeightAndVolumeIfChanged(PrevWeight, PrevVolume);
    return true;
}

bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity)
{
    const int32 SlotIndex = FindSlotWithItemID(ItemID);
    return RemoveItem(SlotIndex, Quantity);
}

bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex)
        return false;

    FInventoryItem& From = Items[FromIndex];
    FInventoryItem& To   = Items[ToIndex];
    if (!From.IsValid())
        return false;

    if (To.IsValid() && From.ItemData == To.ItemData && From.IsStackable())
    {
        const int32 MaxAdd = To.ItemData.Get() ? To.ItemData.Get()->MaxStackSize - To.Quantity : 0;
        const int32 TransferQty = FMath::Clamp(From.Quantity, 0, MaxAdd);
        if (TransferQty <= 0) return false;

        To.Quantity   += TransferQty;
        From.Quantity -= TransferQty;
        if (From.Quantity <= 0)
        {
            From = FInventoryItem();
        }
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
    const int32 Quantity = Item.Quantity;

    const float PrevWeight = GetCurrentWeight();
    const float PrevVolume = GetCurrentVolume();

    const bool bAdded = TargetInventory->TryAddItem(ItemData, Quantity);
    if (bAdded)
    {
        Items[FromIndex] = FInventoryItem();
        OnItemTransferSuccess.Broadcast(Item);

        NotifySlotChanged(FromIndex);
        NotifyInventoryChanged();
        UpdateItemIndexes();

        TargetInventory->NotifyInventoryChanged();

        BroadcastWeightAndVolumeIfChanged(PrevWeight, PrevVolume);
        return true;
    }
    return false;
}

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (HasValidOwnerAuthority(this))
    {
        return AddItem(ItemData, Quantity);
    }
    else
    {
        ServerAddItem(ItemData, Quantity);
        // Optimistic return to keep existing UI behavior
        return true;
    }
}

bool UInventoryComponent::TryRemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (HasValidOwnerAuthority(this))
        return RemoveItem(SlotIndex, Quantity);

    ServerRemoveItem(SlotIndex, Quantity);
    return false;
}

bool UInventoryComponent::TryMoveItem(int32 FromIndex, int32 ToIndex)
{
    if (HasValidOwnerAuthority(this))
        return MoveItem(FromIndex, ToIndex);

    ServerMoveItem(FromIndex, ToIndex);
    return false;
}

bool UInventoryComponent::TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (HasValidOwnerAuthority(this))
        return TransferItemToInventory(FromIndex, TargetInventory);

    ServerTransferItem(FromIndex, TargetInventory);
    return false;
}

bool UInventoryComponent::RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
    if (!SourceInventory || !TargetInventory)
        return false;

    if (HasValidOwnerAuthority(this))
    {
        const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
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
            if (const UItemDataAsset* Data = Item.ItemData.Get())
            {
                Total += Data->Weight * Item.Quantity;
            }
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
            if (const UItemDataAsset* Data = Item.ItemData.Get())
            {
                Total += Data->Volume * Item.Quantity;
            }
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
    {
        if (!Items[i].IsValid())
            return i;
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
    if (!ItemData) return INDEX_NONE;

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (!Slot.IsValid()) continue;

        if (Slot.ItemData == ItemData)
        {
            if (!IsSlotLimited()) // unlimited/weight/volume mode ignores stack cap
                return i;

            const int32 MaxStack = ItemData->MaxStackSize;
            if (Slot.Quantity < MaxStack)
                return i;
        }
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        const UItemDataAsset* Data = Slot.ItemData.Get();
        if (Data && Data->ItemIDTag == ItemID)
            return i;
    }
    return INDEX_NONE;
}

FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
    const int32 SlotIndex = FindSlotWithItemID(ItemID);
    return (SlotIndex != INDEX_NONE) ? Items[SlotIndex] : FInventoryItem();
}

// --- UI Helper ---
int32 UInventoryComponent::GetNumUISlots() const
{
    return MaxSlots;
}

void UInventoryComponent::GetUISlotInfo(TArray<int32>& OutSlotIndices, TArray<UItemDataAsset*>& OutItemData, TArray<int32>& OutQuantities) const
{
    OutSlotIndices.Reset(Items.Num());
    OutItemData.Reset(Items.Num());
    OutQuantities.Reset(Items.Num());

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        OutSlotIndices.Add(i);

        // PIE vs Standalone: if soft ref not loaded in Standalone, try a gentle sync load
        UItemDataAsset* Data = Items[i].ItemData.Get();
        if (!Data && Items[i].ItemData.IsValid())
        {
            Data = Items[i].ItemData.LoadSynchronous();
        }

        OutItemData.Add(Data);
        OutQuantities.Add(Items[i].Quantity);
    }
}

// --- Delegates / bookkeeping ---
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

void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots)
{
    MaxSlots = FMath::Clamp(NewMaxSlots, 1, 1000);
    AdjustSlotCountIfNeeded();
    NotifyInventoryChanged();
}

void UInventoryComponent::AdjustSlotCountIfNeeded()
{
    int32 TargetSlots = MaxSlots;

    if (!IsSlotLimited())
    {
        if (Items.Num() > TargetSlots)
        {
            TargetSlots = Items.Num() + 2;
        }
        else
        {
            TargetSlots += 2;
        }
    }
    else
    {
        TargetSlots = MaxSlots;
    }

    Items.SetNum(TargetSlots);
    UpdateItemIndexes();
}

// --- Replication ---
void UInventoryComponent::OnRep_InventoryItems()
{
    NotifyInventoryChanged();
    // Fire weight/volume updates for UI after replication
    OnWeightChanged.Broadcast(GetCurrentWeight());
    OnVolumeChanged.Broadcast(GetCurrentVolume());
}

void UInventoryComponent::BroadcastWeightAndVolumeIfChanged(float PrevWeight, float PrevVolume) const
{
    const float NewWeight = GetCurrentWeight();
    const float NewVolume = GetCurrentVolume();

    if (!FMath::IsNearlyEqual(NewWeight, PrevWeight))
    {
        OnWeightChanged.Broadcast(NewWeight);
    }
    if (!FMath::IsNearlyEqual(NewVolume, PrevVolume))
    {
        OnVolumeChanged.Broadcast(NewVolume);
    }
}

// --- RPCs ---
void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!HasValidOwnerAuthority(this) || !ItemData || Quantity <= 0) return;
    const bool bSuccess = AddItem(ItemData, Quantity);
    ClientAddItemResponse(bSuccess);
}
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* ItemData, int32 Quantity) { return ItemData != nullptr && Quantity > 0; }

void UInventoryComponent::ClientAddItemResponse_Implementation(bool /*bSuccess*/) {}

void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex, int32 Quantity)
{
    if (!HasValidOwnerAuthority(this)) return;
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0) return;
    RemoveItem(SlotIndex, Quantity);
}
bool UInventoryComponent::ServerRemoveItem_Validate(int32 SlotIndex, int32 Quantity) { return Quantity > 0; }

void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID, int32 Quantity)
{
    if (!HasValidOwnerAuthority(this) || Quantity <= 0) return;
    RemoveItemByID(ItemID, Quantity);
}
bool UInventoryComponent::ServerRemoveItemByID_Validate(FGameplayTag /*ItemID*/, int32 Quantity) { return Quantity > 0; }

void UInventoryComponent::ClientRemoveItemResponse_Implementation(bool /*bSuccess*/) {}

void UInventoryComponent::ServerMoveItem_Implementation(int32 FromIndex, int32 ToIndex)
{
    if (!HasValidOwnerAuthority(this)) return;
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex) return;
    MoveItem(FromIndex, ToIndex);
}
bool UInventoryComponent::ServerMoveItem_Validate(int32 FromIndex, int32 ToIndex) { return FromIndex != ToIndex; }

void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (!HasValidOwnerAuthority(this) || !TargetInventory) return;
    if (!Items.IsValidIndex(FromIndex)) return;
    TransferItemToInventory(FromIndex, TargetInventory);
}
bool UInventoryComponent::ServerTransferItem_Validate(int32 /*FromIndex*/, UInventoryComponent* TargetInventory) { return TargetInventory != nullptr; }

void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex, UInventoryComponent* SourceInventory)
{
    if (!HasValidOwnerAuthority(this) || !SourceInventory || SourceInventory == this) return;

    const FInventoryItem FromSlot = SourceInventory->GetItem(FromIndex);
    if (!FromSlot.IsValid()) return;

    UItemDataAsset* Asset = FromSlot.ItemData.Get();
    const int32 Quantity = FromSlot.Quantity;

    if (Asset && TryAddItem(Asset, Quantity))
    {
        SourceInventory->RemoveItem(FromIndex, Quantity);
    }
}
bool UInventoryComponent::ServerPullItem_Validate(int32 /*FromIndex*/, UInventoryComponent* SourceInventory) { return SourceInventory != nullptr; }

void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
    if (!HasValidOwnerAuthority(this) || !SourceInventory || !TargetInventory) return;

    const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
    if (!Item.IsValid()) return;

    if (TargetInventory->TryAddItem(Item.ItemData.Get(), Item.Quantity))
    {
        SourceInventory->RemoveItem(SourceIndex, Item.Quantity);
    }
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* SourceInventory, int32 /*SourceIndex*/, UInventoryComponent* TargetInventory, int32 /*TargetIndex*/)
{
    return SourceInventory != nullptr && TargetInventory != nullptr;
}

int32 UInventoryComponent::GetNumOccupiedSlots() const
{
    int32 Count = 0;
    for (const FInventoryItem& Item : Items)
    {
        if (Item.IsValid()) ++Count;
    }
    return Count;
}

int32 UInventoryComponent::GetNumItemsOfType(FGameplayTag ItemID) const
{
    int32 Count = 0;
    for (const FInventoryItem& Item : Items)
    {
        if (Item.IsValid() && Item.ItemData.IsValid())
        {
            const UItemDataAsset* Data = Item.ItemData.Get();
            if (Data && Data->ItemIDTag == ItemID)
            {
                Count += Item.Quantity;
            }
        }
    }
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
        return true;
    return AllowedItemIDs.Contains(ItemData->ItemIDTag);
}

// --- Sort by Name ---
void UInventoryComponent::ServerSortInventoryByName_Implementation()
{
    SortInventoryByName();
}
bool UInventoryComponent::ServerSortInventoryByName_Validate() { return true; }

void UInventoryComponent::RequestSortInventoryByName()
{
    if (HasValidOwnerAuthority(this))
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
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();

        const bool AValid = A.IsValid() && DA;
        const bool BValid = B.IsValid() && DB;

        if (AValid != BValid)
        {
            return BValid; // invalid goes last
        }
        if (!AValid && !BValid)
        {
            return false; // keep relative order
        }
        const FString NameA = DA->Name.ToString();
        const FString NameB = DB->Name.ToString();
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
        const UItemDataAsset* AData = A.ItemData.Get();
        const UItemDataAsset* BData = B.ItemData.Get();

        const bool AValid = A.IsValid() && AData;
        const bool BValid = B.IsValid() && BData;

        if (AValid != BValid) return BValid;
        if (!AValid && !BValid) return false;

        return AData->Rarity.ToString() < BData->Rarity.ToString();
    });

    UpdateItemIndexes();
    NotifyInventoryChanged();
}

void UInventoryComponent::RequestSortInventoryByRarity()
{
    if (HasValidOwnerAuthority(this))
    {
        SortInventoryByRarity();
    }
    else
    {
        ServerSortInventoryByRarity();
    }
}
void UInventoryComponent::ServerSortInventoryByRarity_Implementation() { SortInventoryByRarity(); }
bool UInventoryComponent::ServerSortInventoryByRarity_Validate() { return true; }

// --- Sort by Type ---
void UInventoryComponent::SortInventoryByType()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* AData = A.ItemData.Get();
        const UItemDataAsset* BData = B.ItemData.Get();

        const bool AValid = A.IsValid() && AData;
        const bool BValid = B.IsValid() && BData;

        if (AValid != BValid) return BValid;
        if (!AValid && !BValid) return false;

        return AData->ItemType.ToString() < BData->ItemType.ToString();
    });

    UpdateItemIndexes();
    NotifyInventoryChanged();
}

void UInventoryComponent::RequestSortInventoryByType()
{
    if (HasValidOwnerAuthority(this))
    {
        SortInventoryByType();
    }
    else
    {
        ServerSortInventoryByType();
    }
}
void UInventoryComponent::ServerSortInventoryByType_Implementation() { SortInventoryByType(); }
bool UInventoryComponent::ServerSortInventoryByType_Validate() { return true; }

// --- Sort by Category ---
void UInventoryComponent::SortInventoryByCategory()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* AData = A.ItemData.Get();
        const UItemDataAsset* BData = B.ItemData.Get();

        const bool AValid = A.IsValid() && AData;
        const bool BValid = B.IsValid() && BData;

        if (AValid != BValid) return BValid;
        if (!AValid && !BValid) return false;

        return AData->ItemCategory.ToString() < BData->ItemCategory.ToString();
    });

    UpdateItemIndexes();
    NotifyInventoryChanged();
}

void UInventoryComponent::RequestSortInventoryByCategory()
{
    if (HasValidOwnerAuthority(this))
    {
        SortInventoryByCategory();
    }
    else
    {
        ServerSortInventoryByCategory();
    }
}
void UInventoryComponent::ServerSortInventoryByCategory_Implementation() { SortInventoryByCategory(); }
bool UInventoryComponent::ServerSortInventoryByCategory_Validate() { return true; }

// --- Filters ---
TArray<FInventoryItem> UInventoryComponent::FilterItemsByRarity(FGameplayTag RarityTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        const UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->Rarity == RarityTag)
        {
            Results.Add(Item);
        }
    }
    return Results;
}

TArray<FInventoryItem> UInventoryComponent::FilterItemsByCategory(FGameplayTag CategoryTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        const UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemCategory == CategoryTag)
        {
            Results.Add(Item);
        }
    }
    return Results;
}

TArray<FInventoryItem> UInventoryComponent::FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        const UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemSubCategory == SubCategoryTag)
        {
            Results.Add(Item);
        }
    }
    return Results;
}

TArray<FInventoryItem> UInventoryComponent::FilterItemsByType(FGameplayTag TypeTag) const
{
    TArray<FInventoryItem> Results;
    for (const FInventoryItem& Item : Items)
    {
        if (!Item.IsValid()) continue;
        const UItemDataAsset* Data = Item.ItemData.Get();
        if (Data && Data->ItemType == TypeTag)
        {
            Results.Add(Item);
        }
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
        const UItemDataAsset* Data = Item.ItemData.Get();
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
        return false;

    const int32 FreeSlot = FindFreeSlot();
    if (FreeSlot == INDEX_NONE)
        return false;

    const float PrevWeight = GetCurrentWeight();
    const float PrevVolume = GetCurrentVolume();

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

    BroadcastWeightAndVolumeIfChanged(PrevWeight, PrevVolume);
    return true;
}

void UInventoryComponent::ServerSplitStack_Implementation(int32 SlotIndex, int32 SplitQuantity)
{
    if (!HasValidOwnerAuthority(this)) return;
    SplitStack(SlotIndex, SplitQuantity);
}
bool UInventoryComponent::ServerSplitStack_Validate(int32 SlotIndex, int32 SplitQuantity)
{
    return SplitQuantity > 0 && SlotIndex >= 0;
}
