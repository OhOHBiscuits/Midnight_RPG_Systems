// InventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Inventory/InventoryItem.h"
#include "InventoryComponent.generated.h"

// Forward declaration
class UItemDataAsset;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Core inventory functions
    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryItem GetItem(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    float GetCurrentWeight() const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    float GetCurrentVolume() const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    void SetMaxCarryWeight(float NewMaxWeight);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    void SetMaxCarryVolume(float NewMaxVolume);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 FindFreeSlot() const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 FindStackableSlot(UItemDataAsset* ItemData) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool AddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool MoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 FindSlotWithItemID(FGameplayTag ItemID) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryItem GetItemByID(FGameplayTag ItemID) const;

    // Try versions for client/server convenience
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryAddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryRemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryMoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Inventory")
    bool IsInventoryFull() const;

    // Notifiers for UI
    UFUNCTION()
    void OnRep_InventoryItems();

    void NotifySlotChanged(int32 SlotIndex);
    void NotifyInventoryChanged();

    // Replication
    UPROPERTY(ReplicatedUsing=OnRep_InventoryItems)
    TArray<FInventoryItem> Items;

    // Delegates for UI
    UPROPERTY(BlueprintAssignable, Category="Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

    UPROPERTY(BlueprintAssignable, Category="Inventory")
    FOnInventoryChanged OnInventoryChanged;

    // Networking
    UFUNCTION(Server, Reliable)
    void ServerAddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(Client, Reliable)
    void ClientAddItemResponse(bool bSuccess);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(Client, Reliable)
    void ClientRemoveItemResponse(bool bSuccess);

    UFUNCTION(Server, Reliable)
    void ServerMoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(Server, Reliable)
    void ServerTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(Server, Reliable)
    void ServerPullItem(int32 FromIndex, UInventoryComponent* SourceInventory);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItemByID(FGameplayTag ItemID, int32 Quantity);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    int32 MaxSlots = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    float MaxCarryWeight = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    float MaxCarryVolume = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    FGameplayTag InventoryBehaviorTag;
};
