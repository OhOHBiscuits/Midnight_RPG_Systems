// InventoryComponent.cpp
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    Items.SetNum(MaxSlots);

    if (!InventoryBehaviorTag.IsValid())
    {
        InventoryBehaviorTag = FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.WeightLimited"), false);
    }
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryComponent, Items);
}

FInventoryItem UInventoryComponent::GetItem(int32 SlotIndex) const
{
    return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex] : FInventoryItem();
}

void UInventoryComponent::OnRep_InventoryItems()
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        OnInventoryUpdated.Broadcast(i);
    }
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::NotifySlotChanged(int32 SlotIndex)
{
    OnInventoryUpdated.Broadcast(SlotIndex);
}

void UInventoryComponent::NotifyInventoryChanged()
{
    OnInventoryChanged.Broadcast();
}

float UInventoryComponent::GetCurrentWeight() const
{
    float Total = 0.f;
    for (const FInventoryItem& Slot : Items)
    {
        if (Slot.IsValid())
        {
            UItemDataAsset* Data = Slot.ItemData.IsValid() ? Slot.ItemData.Get() : Slot.ItemData.LoadSynchronous();
            if (Data)
            {
                Total += Data->Weight * Slot.Quantity;
            }
        }
    }
    return Total;
}

float UInventoryComponent::GetCurrentVolume() const
{
    float TotalVol = 0.f;
    for (const FInventoryItem& Slot : Items)
    {
        if (Slot.IsValid())
        {
            UItemDataAsset* Data = Slot.ItemData.IsValid() ? Slot.ItemData.Get() : Slot.ItemData.LoadSynchronous();
            if (Data)
            {
                TotalVol += Data->Volume * Slot.Quantity;
            }
        }
    }
    return TotalVol;
}

void UInventoryComponent::SetMaxCarryWeight(float NewMaxWeight)
{
    MaxCarryWeight = NewMaxWeight;
}

void UInventoryComponent::SetMaxCarryVolume(float NewMaxVolume)
{
    MaxCarryVolume = NewMaxVolume;
}

int32 UInventoryComponent::FindFreeSlot() const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (!Items[i].IsValid())
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
    if (!ItemData) return INDEX_NONE;
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (Slot.IsValid() && Slot.ItemData == ItemData && Slot.Quantity < ItemData->MaxStackSize)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (!ItemData || Quantity <= 0)
        return false;

    if (InventoryBehaviorTag == FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.WeightLimited"), false))
    {
        if (GetCurrentWeight() + ItemData->Weight * Quantity > MaxCarryWeight)
            return false;
    }
    else if (InventoryBehaviorTag == FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.VolumeLimited"), false))
    {
        if (GetCurrentVolume() + ItemData->Volume * Quantity > MaxCarryVolume)
            return false;
    }

    int32 SlotIndex = FindStackableSlot(ItemData);
    if (SlotIndex == INDEX_NONE)
        SlotIndex = FindFreeSlot();
    if (SlotIndex == INDEX_NONE)
        return false;

    FInventoryItem& Slot = Items[SlotIndex];
    if (!Slot.IsValid())
    {
        Slot.ItemData  = ItemData;
        Slot.Quantity  = Quantity;
        Slot.SlotIndex = SlotIndex;
    }
    else
    {
        Slot.Quantity = FMath::Clamp(Slot.Quantity + Quantity, 0, ItemData->MaxStackSize);
    }

    NotifySlotChanged(SlotIndex);
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0)
        return false;

    FInventoryItem& Slot = Items[SlotIndex];
    if (!Slot.IsValid() || Slot.Quantity < Quantity)
        return false;

    Slot.Quantity -= Quantity;
    if (Slot.Quantity <= 0)
        Slot = FInventoryItem();

    NotifySlotChanged(SlotIndex);
    NotifyInventoryChanged();
    return true;
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
        int32 TransferQty = FMath::Min(From.Quantity, To.ItemData->MaxStackSize - To.Quantity);
        To.Quantity   += TransferQty;
        From.Quantity -= TransferQty;
        if (From.Quantity <= 0)
            From = FInventoryItem();

        NotifySlotChanged(FromIndex);
        NotifySlotChanged(ToIndex);
        NotifyInventoryChanged();
        return true;
    }

    Swap(From, To);
    From.SlotIndex = FromIndex;
    To.SlotIndex   = ToIndex;

    NotifySlotChanged(FromIndex);
    NotifySlotChanged(ToIndex);
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (!Items.IsValidIndex(FromIndex) || !TargetInventory || TargetInventory == this)
        return false;

    FInventoryItem FromSlot = GetItem(FromIndex);
    if (!FromSlot.IsValid())
        return false;

    UItemDataAsset* Asset = FromSlot.ItemData.IsValid() ? FromSlot.ItemData.Get() : FromSlot.ItemData.LoadSynchronous();
    if (!Asset)
        return false;

    bool bSuccess = TargetInventory->AddItem(Asset, FromSlot.Quantity);
    if (bSuccess)
        RemoveItem(FromIndex, FromSlot.Quantity);

    return bSuccess;
}

int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        const FInventoryItem& Slot = Items[i];
        if (Slot.IsValid())
        {
            UItemDataAsset* Data = Slot.ItemData.IsValid() ? Slot.ItemData.Get() : Slot.ItemData.LoadSynchronous();
            if (Data && Data->ItemIDTag == ItemID)
                return i;
        }
    }
    return INDEX_NONE;
}

FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
    int32 SlotIndex = FindSlotWithItemID(ItemID);
    return SlotIndex != INDEX_NONE ? Items[SlotIndex] : FInventoryItem();
}

void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData, int32 Quantity)
{
    bool bResult = AddItem(ItemData, Quantity);
    ClientAddItemResponse(bResult);
}

void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex, int32 Quantity)
{
    bool bResult = RemoveItem(SlotIndex, Quantity);
    ClientRemoveItemResponse(bResult);
}

void UInventoryComponent::ServerMoveItem_Implementation( int32 FromIndex, int32 ToIndex)
{
    MoveItem(FromIndex, ToIndex);
}

void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    TransferItemToInventory(FromIndex, TargetInventory);
}

void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex, UInventoryComponent* SourceInventory)
{
    if (!SourceInventory || SourceInventory == this)
        return;

    FInventoryItem FromSlot = SourceInventory->GetItem(FromIndex);
    if (!FromSlot.IsValid())
        return;

    UItemDataAsset* Asset = FromSlot.ItemData.IsValid() ? FromSlot.ItemData.Get() : FromSlot.ItemData.LoadSynchronous();
    int32 Quantity = FromSlot.Quantity;

    if (Asset && AddItem(Asset, Quantity))
        SourceInventory->RemoveItem(FromIndex, Quantity);
}

void UInventoryComponent::ClientAddItemResponse_Implementation(bool bSuccess) {}
void UInventoryComponent::ClientRemoveItemResponse_Implementation(bool bSuccess) {}

void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID, int32 Quantity)
{
    int32 SlotIndex = FindSlotWithItemID(ItemID);
    if (SlotIndex != INDEX_NONE)
        RemoveItem(SlotIndex, Quantity);
}

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData, int32 Quantity)
{
    if (GetOwner()->HasAuthority())
    {
        return AddItem(ItemData, Quantity);
    }
    ServerAddItem(ItemData, Quantity);
    return true;
}

bool UInventoryComponent::TryRemoveItem(int32 SlotIndex, int32 Quantity)
{
    if (GetOwner()->HasAuthority())
    {
        return RemoveItem(SlotIndex, Quantity);
    }
    ServerRemoveItem(SlotIndex, Quantity);
    return true;
}

bool UInventoryComponent::TryMoveItem(int32 FromIndex, int32 ToIndex)
{
    if (GetOwner()->HasAuthority())
    {
        return MoveItem(FromIndex, ToIndex);
    }
    ServerMoveItem(FromIndex, ToIndex);
    return true;
}

bool UInventoryComponent::TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory)
{
    if (GetOwner()->HasAuthority())
    {
        return TransferItemToInventory(FromIndex, TargetInventory);
    }
    ServerTransferItem(FromIndex, TargetInventory);
    return true;
}

bool UInventoryComponent::IsInventoryFull() const
{
    const bool bNoSlots = (FindFreeSlot() == INDEX_NONE);

    if (InventoryBehaviorTag == FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.WeightLimited"), false))
        return GetCurrentWeight() >= MaxCarryWeight;

    if (InventoryBehaviorTag == FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.VolumeLimited"), false))
        return GetCurrentVolume() >= MaxCarryVolume;

    if (InventoryBehaviorTag == FGameplayTag::RequestGameplayTag(FName("Inventory.Behavior.NoLimit"), false))
        return false;

    return bNoSlots;
}
