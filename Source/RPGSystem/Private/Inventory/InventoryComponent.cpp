#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryAccess.h"

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
bool UInventoryComponent::IsVolumeLimited() const { return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Volume"))); }
bool UInventoryComponent::IsWeightLimited() const { return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Weight"))); }
bool UInventoryComponent::IsSlotLimited()   const { return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.Slot"))); }
bool UInventoryComponent::IsUnlimited()     const { return InventoryBehaviorTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.None"))); }

// --- Actions ---
bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!ItemData || Quantity <= 0) return false;
    if (!CanAcceptItem(ItemData))   return false;

    const int32 StackSlot = FindStackableSlot(ItemData);
    const int32 FreeSlot  = (StackSlot == INDEX_NONE) ? FindFreeSlot() : StackSlot;
    if (FreeSlot == INDEX_NONE) { OnInventoryFull.Broadcast(true); return false; }

    if (IsUnlimited())
    {
        if (Items[FreeSlot].IsValid()) Items[FreeSlot].Quantity += Quantity;
        else Items[FreeSlot] = FInventoryItem(ItemData, Quantity, FreeSlot);

        OnItemAdded.Broadcast(Items[FreeSlot], Quantity);
        if (bAutoSort) RequestSortInventoryByName();
        NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
        OnInventoryFull.Broadcast(false);
        return true;
    }

    if (IsVolumeLimited())
    {
        const float NewVolume = GetCurrentVolume() + (ItemData->Volume * Quantity);
        if (NewVolume > MaxCarryVolume) { OnInventoryFull.Broadcast(true); return false; }
    }
    if (IsWeightLimited())
    {
        const float NewWeight = GetCurrentWeight() + (ItemData->Weight * Quantity);
        if (NewWeight > MaxCarryWeight) { OnInventoryFull.Broadcast(true); return false; }
    }

    int32 AddQty = Quantity;
    if (IsSlotLimited() && Items[FreeSlot].IsValid())
    {
        AddQty = FMath::Min(Quantity, ItemData->MaxStackSize - Items[FreeSlot].Quantity);
        if (AddQty <= 0) { OnInventoryFull.Broadcast(true); return false; }
    }

    if (Items[FreeSlot].IsValid()) Items[FreeSlot].Quantity += AddQty;
    else Items[FreeSlot] = FInventoryItem(ItemData, AddQty, FreeSlot);

    OnItemAdded.Broadcast(Items[FreeSlot], AddQty);
    if (bAutoSort) RequestSortInventoryByName();
    NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
    OnInventoryFull.Broadcast(IsInventoryFull());
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0) return false;
    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity < Quantity) return false;

    OnItemRemoved.Broadcast(Item);
    Item.Quantity -= Quantity;
    if (Item.Quantity <= 0) Items[SlotIndex] = FInventoryItem();

    if (bAutoSort) RequestSortInventoryByName();
    NotifySlotChanged(SlotIndex); NotifyInventoryChanged(); UpdateItemIndexes();
    return true;
}

bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity)
{
    const int32 SlotIndex = FindSlotWithItemID(ItemID);
    return RemoveItem(SlotIndex, Quantity);
}

bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex)
{
    if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex) return false;

    FInventoryItem& From = Items[FromIndex];
    FInventoryItem& To   = Items[ToIndex];
    if (!From.IsValid()) return false;

    if (To.IsValid() && From.ItemData == To.ItemData && From.IsStackable())
    {
        const int32 TransferQty = FMath::Min(From.Quantity, To.ItemData.Get()->MaxStackSize - To.Quantity);
        if (TransferQty <= 0) return false;
        To.Quantity += TransferQty;
        From.Quantity -= TransferQty;
        if (From.Quantity <= 0) From = FInventoryItem();
    }
    else
    {
        Swap(From, To);
    }

    NotifySlotChanged(FromIndex); NotifySlotChanged(ToIndex); NotifyInventoryChanged(); UpdateItemIndexes();
    return true;
}

bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (!TargetInventory || !Items.IsValidIndex(FromIndex)) return false;

    FInventoryItem& Item = Items[FromIndex];
    if (!Item.IsValid()) return false;

    UItemDataAsset* ItemData = Item.ItemData.Get();
    const int32 Quantity = Item.Quantity;

    if (TargetInventory->TryAddItem(ItemData, Quantity))
    {
        Items[FromIndex] = FInventoryItem();
        OnItemTransferSuccess.Broadcast(Item);
        NotifySlotChanged(FromIndex); NotifyInventoryChanged(); UpdateItemIndexes();
        TargetInventory->NotifyInventoryChanged();
        return true;
    }
    return false;
}

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) return AddItem(ItemData, Quantity);
    ServerAddItem(ItemData, Quantity);
    return true;
}

bool UInventoryComponent::TryRemoveItem(int32 SlotIndex, int32 Quantity)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) return RemoveItem(SlotIndex, Quantity);
    ServerRemoveItem(SlotIndex, Quantity);
    return false;
}

bool UInventoryComponent::TryMoveItem(int32 FromIndex, int32 ToIndex)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) return MoveItem(FromIndex, ToIndex);
    ServerMoveItem(FromIndex, ToIndex);
    return false;
}

bool UInventoryComponent::TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) return TransferItemToInventory(FromIndex, TargetInventory);
    ServerTransferItem(FromIndex, TargetInventory);
    return false;
}

bool UInventoryComponent::RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
    if (!SourceInventory || !TargetInventory) return false;
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
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
    Server_TransferItem(SourceInventory, SourceIndex, TargetInventory, TargetIndex);
    return false;
}

// --- Split ---
bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity)
{
    if (!Items.IsValidIndex(SlotIndex) || SplitQuantity <= 0) return false;
    FInventoryItem& Item = Items[SlotIndex];
    if (!Item.IsValid() || Item.Quantity <= SplitQuantity) return false;

    const int32 FreeSlot = FindFreeSlot();
    if (FreeSlot == INDEX_NONE) return false;

    Item.Quantity -= SplitQuantity;
    Items[FreeSlot] = FInventoryItem(Item.ItemData.Get(), SplitQuantity, FreeSlot);

    if (bAutoSort) RequestSortInventoryByName();
    NotifySlotChanged(SlotIndex); NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
    return true;
}

void UInventoryComponent::ServerSplitStack_Implementation(int32 SlotIndex, int32 SplitQuantity)
{
    SplitStack(SlotIndex, SplitQuantity);
}
bool UInventoryComponent::ServerSplitStack_Validate(int32 SlotIndex, int32 SplitQuantity) { return SplitQuantity > 0 && SlotIndex >= 0; }

bool UInventoryComponent::TrySplitStack(int32 SlotIndex, int32 SplitQuantity)
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
    {
        return SplitStack(SlotIndex, SplitQuantity);
    }
    ServerSplitStack(SlotIndex, SplitQuantity);
    return false;
}

// --- Sort (Name) ---
void UInventoryComponent::SortInventoryByName()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();

        if (!A.IsValid() && !B.IsValid()) return false;
        if (!A.IsValid()) return false; // empty last
        if (!B.IsValid()) return true;

        const FString NameA = DA ? DA->Name.ToString() : FString();
        const FString NameB = DB ? DB->Name.ToString() : FString();
        return NameA < NameB;
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByName_Implementation() { SortInventoryByName(); }
bool UInventoryComponent::ServerSortInventoryByName_Validate() { return true; }
void UInventoryComponent::RequestSortInventoryByName()
{
    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority()) SortInventoryByName();
    else ServerSortInventoryByName();
}

// --- Sort (Rarity/Type/Category) ---
void UInventoryComponent::SortInventoryByRarity()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();
        if (!A.IsValid() && !B.IsValid()) return false;
        if (!A.IsValid()) return false;
        if (!B.IsValid()) return true;
        return (DA ? DA->Rarity.ToString() : FString()) < (DB ? DB->Rarity.ToString() : FString());
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::SortInventoryByType()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();
        if (!A.IsValid() && !B.IsValid()) return false;
        if (!A.IsValid()) return false;
        if (!B.IsValid()) return true;
        return (DA ? DA->ItemType.ToString() : FString()) < (DB ? DB->ItemType.ToString() : FString());
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}
void UInventoryComponent::SortInventoryByCategory()
{
    Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
    {
        const UItemDataAsset* DA = A.ItemData.Get();
        const UItemDataAsset* DB = B.ItemData.Get();
        if (!A.IsValid() && !B.IsValid()) return false;
        if (!A.IsValid()) return false;
        if (!B.IsValid()) return true;
        return (DA ? DA->ItemCategory.ToString() : FString()) < (DB ? DB->ItemCategory.ToString() : FString());
    });
    UpdateItemIndexes();
    NotifyInventoryChanged();
}

void UInventoryComponent::RequestSortInventoryByRarity()   { if (GetOwner() && GetOwner()->HasAuthority()) SortInventoryByRarity();   else ServerSortInventoryByRarity(); }
void UInventoryComponent::RequestSortInventoryByType()     { if (GetOwner() && GetOwner()->HasAuthority()) SortInventoryByType();     else ServerSortInventoryByType(); }
void UInventoryComponent::RequestSortInventoryByCategory() { if (GetOwner() && GetOwner()->HasAuthority()) SortInventoryByCategory(); else ServerSortInventoryByCategory(); }

void UInventoryComponent::ServerSortInventoryByRarity_Implementation()   { SortInventoryByRarity(); }
bool UInventoryComponent::ServerSortInventoryByRarity_Validate() { return true; }

void UInventoryComponent::ServerSortInventoryByType_Implementation()     { SortInventoryByType(); }
bool UInventoryComponent::ServerSortInventoryByType_Validate() { return true; }

void UInventoryComponent::ServerSortInventoryByCategory_Implementation() { SortInventoryByCategory(); }
bool UInventoryComponent::ServerSortInventoryByCategory_Validate() { return true; }

// --- Queries ---
bool UInventoryComponent::IsInventoryFull() const
{
    if (IsUnlimited()) return false;
    if (IsSlotLimited())   return FindFreeSlot() == INDEX_NONE;
    if (IsWeightLimited()) return GetCurrentWeight() >= MaxCarryWeight;
    if (IsVolumeLimited()) return GetCurrentVolume() >= MaxCarryVolume;
    return false;
}

float UInventoryComponent::GetCurrentWeight() const
{
    float Total = 0.f;
    for (const FInventoryItem& Item : Items)
        if (Item.IsValid())
            if (const UItemDataAsset* Data = Item.ItemData.Get())
                Total += Data->Weight * Item.Quantity;
    return Total;
}

float UInventoryComponent::GetCurrentVolume() const
{
    float Total = 0.f;
    for (const FInventoryItem& Item : Items)
        if (Item.IsValid())
            if (const UItemDataAsset* Data = Item.ItemData.Get())
                Total += Data->Volume * Item.Quantity;
    return Total;
}

FInventoryItem UInventoryComponent::GetItem(int32 SlotIndex) const
{
    return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex] : FInventoryItem();
}

int32 UInventoryComponent::FindFreeSlot() const
{
    for (int32 i=0;i<Items.Num();++i) if (!Items[i].IsValid()) return i;
    return INDEX_NONE;
}

int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
    if (!ItemData) return INDEX_NONE;
    for (int32 i=0;i<Items.Num();++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (Slot.IsValid() && Slot.ItemData == ItemData && (IsSlotLimited() ? Slot.Quantity < ItemData->MaxStackSize : true))
            return i;
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
    for (int32 i=0;i<Items.Num();++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (const UItemDataAsset* Data = Slot.ItemData.Get())
            if (Data->ItemIDTag == ItemID)
                return i;
    }
    return INDEX_NONE;
}

FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
    const int32 Slot = FindSlotWithItemID(ItemID);
    return (Slot != INDEX_NONE) ? Items[Slot] : FInventoryItem();
}

int32 UInventoryComponent::GetNumOccupiedSlots() const
{
    int32 Count = 0;
    for (const FInventoryItem& It : Items)
        if (It.IsValid()) ++Count;
    return Count;
}

int32 UInventoryComponent::GetNumItemsOfType(FGameplayTag ItemID) const
{
    int32 Count = 0;
    for (const FInventoryItem& It : Items)
        if (It.IsValid() && It.ItemData.IsValid())
            if (It.ItemData.Get()->ItemIDTag == ItemID)
                Count += It.Quantity;
    return Count;
}

// --- UI helpers ---
int32 UInventoryComponent::GetNumUISlots() const
{
    return MaxSlots;
}

void UInventoryComponent::GetUISlotInfo(TArray<int32>& OutSlotIndices, TArray<UItemDataAsset*>& OutItemData, TArray<int32>& OutQuantities) const
{
    OutSlotIndices.Reset();
    OutItemData.Reset();
    OutQuantities.Reset();

    for (int32 i=0;i<Items.Num();++i)
    {
        OutSlotIndices.Add(i);
        OutItemData.Add(Items[i].ItemData.Get());
        OutQuantities.Add(Items[i].Quantity);
    }
}

// --- Delegates / Notifications ---
void UInventoryComponent::NotifySlotChanged(int32 SlotIndex) { OnInventoryUpdated.Broadcast(SlotIndex); }
void UInventoryComponent::NotifyInventoryChanged() { OnInventoryChanged.Broadcast(); }
void UInventoryComponent::UpdateItemIndexes()
{
    for (int32 i=0;i<Items.Num();++i) Items[i].ItemIndex = i;
}

// --- Settings ---
void UInventoryComponent::SetMaxCarryWeight(float NewMaxWeight)
{
    MaxCarryWeight = FMath::Max(0.f, NewMaxWeight);
    OnWeightChanged.Broadcast(GetCurrentWeight());
}
void UInventoryComponent::SetMaxCarryVolume(float NewMaxVolume)
{
    MaxCarryVolume = FMath::Max(0.f, NewMaxVolume);
    OnVolumeChanged.Broadcast(GetCurrentVolume());
}
void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots)
{
    MaxSlots = FMath::Clamp(NewMaxSlots, 1, 1000);
    AdjustSlotCountIfNeeded();
    NotifyInventoryChanged();
}
void UInventoryComponent::AdjustSlotCountIfNeeded()
{
    int32 Target = MaxSlots;
    if (!IsSlotLimited())
    {
        if (Items.Num() > Target) Target = Items.Num() + 2;
        else Target += 2;
    }
    Items.SetNum(Target);
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
    const bool bSuccess = AddItem(ItemData, Quantity);
    ClientAddItemResponse(bSuccess);
}
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* ItemData, int32 Quantity) { return true; }

void UInventoryComponent::ClientAddItemResponse_Implementation(bool /*bSuccess*/) {}

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

void UInventoryComponent::ClientRemoveItemResponse_Implementation(bool /*bSuccess*/) {}

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
    if (!SourceInventory || SourceInventory == this) return;

    const FInventoryItem FromSlot = SourceInventory->GetItem(FromIndex);
    if (!FromSlot.IsValid()) return;

    UItemDataAsset* Asset = FromSlot.ItemData.Get();
    const int32 Quantity = FromSlot.Quantity;

    if (Asset && TryAddItem(Asset, Quantity))
        SourceInventory->RemoveItem(FromIndex, Quantity);
}
bool UInventoryComponent::ServerPullItem_Validate(int32 FromIndex, UInventoryComponent* SourceInventory) { return true; }

void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 /*TargetIndex*/)
{
    if (!SourceInventory || !TargetInventory) return;
    const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
    if (!Item.IsValid()) return;

    if (TargetInventory->TryAddItem(Item.ItemData.Get(), Item.Quantity))
        SourceInventory->RemoveItem(SourceIndex, Item.Quantity);
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex) { return true; }

// --- Misc ---
bool UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB)
{
    if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB) return false;
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
    if (AllowedItemIDs.Num() == 0) return true;
    return AllowedItemIDs.Contains(ItemData->ItemIDTag);
}

// --- Filters ---
TArray<FInventoryItem> UInventoryComponent::FilterItemsByRarity(FGameplayTag RarityTag) const
{
    TArray<FInventoryItem> Out;
    for (const FInventoryItem& It : Items)
        if (It.IsValid())
            if (const UItemDataAsset* D = It.ItemData.Get())
                if (D->Rarity == RarityTag) Out.Add(It);
    return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByCategory(FGameplayTag CategoryTag) const
{
    TArray<FInventoryItem> Out;
    for (const FInventoryItem& It : Items)
        if (It.IsValid())
            if (const UItemDataAsset* D = It.ItemData.Get())
                if (D->ItemCategory == CategoryTag) Out.Add(It);
    return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const
{
    TArray<FInventoryItem> Out;
    for (const FInventoryItem& It : Items)
        if (It.IsValid())
            if (const UItemDataAsset* D = It.ItemData.Get())
                if (D->ItemSubCategory == SubCategoryTag) Out.Add(It);
    return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByType(FGameplayTag TypeTag) const
{
    TArray<FInventoryItem> Out;
    for (const FInventoryItem& It : Items)
        if (It.IsValid())
            if (const UItemDataAsset* D = It.ItemData.Get())
                if (D->ItemType == TypeTag) Out.Add(It);
    return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll) const
{
    TArray<FInventoryItem> Out;
    for (const FInventoryItem& It : Items)
    {
        if (!It.IsValid()) continue;
        const UItemDataAsset* D = It.ItemData.Get();
        if (!D) continue;

        FGameplayTagContainer ItemTags;
        ItemTags.AddTag(D->Rarity);
        ItemTags.AddTag(D->ItemType);
        ItemTags.AddTag(D->ItemCategory);
        ItemTags.AddTag(D->ItemSubCategory);

        const bool bPass = bMatchAll ? ItemTags.HasAll(Tags) : ItemTags.HasAny(Tags);
        if (bPass) Out.Add(It);
    }
    return Out;
}

static bool CanStoreItemHere(const FInventoryItem& Item, const UInventoryComponent* Target)
{
    if (!Item.bIsHeirloom) return true;
    if (!Target) return false;
    if (!Target->IsOwnerSafeContainer()) return false;
    return Item.OwnerPlayerId.Equals(Target->InventoryOwnerId, ESearchCase::IgnoreCase);
}

static bool CanRemoveItemFromHere(const FInventoryItem& Item, const UInventoryComponent* Source, const UInventoryComponent* Dest)
{
    if (!Item.bIsHeirloom) return true;
    // You can remove it only if it’s going to another owner-safe container owned by same player.
    return Dest && CanStoreItemHere(Item, Dest);
}

static bool CanDestroy(const FInventoryItem& Item)
{
    return !Item.bIsHeirloom; // never destroy heirlooms via normal flows
}
// --- Ownership (CPP) ---
void UInventoryComponent::SetInventoryOwnerId(const FString& NewOwnerId)
{
    // Blueprint-accessible wrapper defaults to locking
    SetInventoryOwnerIdInternal(NewOwnerId, /*bLock=*/true);
}

void UInventoryComponent::SetInventoryOwnerIdInternal(const FString& NewOwnerId, bool bLock)
{
    InventoryOwnerId = NewOwnerId;
    bOwnerLocked = bLock;
}

void UInventoryComponent::SetPrivacyTag(FGameplayTag NewPrivacy)
{
    if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
    {
        PrivacyTag = NewPrivacy;
        OnRep_Privacy();
    }
}

void UInventoryComponent::AddAllowedUserId(const FString& UserId)
{
    if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
    {
        AllowedUserIds.AddUnique(UserId);
        OnRep_Owner();
    }
}

void UInventoryComponent::RemoveAllowedUserId(const FString& UserId)
{
    if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
    {
        AllowedUserIds.Remove(UserId);
        OnRep_Owner();
    }
}
