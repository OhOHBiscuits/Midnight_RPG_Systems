#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryItem.h"
#include "ItemDataAsset.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.generated.h"

// Delegates for UI and system hooks
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeChanged, float, NewVolume);

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RPGSYSTEM_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    // --- 1_Inventory|Actions ---
    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool AddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool RemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool RemoveItemByID(FGameplayTag ItemID, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool MoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool TryAddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool TryRemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool TryMoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, Category = "1_Inventory|Actions")
    virtual bool RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1_Inventory|Type")
    FGameplayTag InventoryTypeTag;

    // --- 2_Inventory|Queries ---
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual bool IsInventoryFull() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual float GetCurrentWeight() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual float GetCurrentVolume() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual FInventoryItem GetItem(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual int32 FindFreeSlot() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual int32 FindStackableSlot(UItemDataAsset* ItemData) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual int32 FindSlotWithItemID(FGameplayTag ItemID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    virtual FInventoryItem GetItemByID(FGameplayTag ItemID) const;

    // ---- NEW: Expose Items array safely for Fuel/Decay etc. ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "2_Inventory|Queries")
    const TArray<FInventoryItem>& GetAllItems() const { return Items; }

    // --- 3_Inventory|Events (Delegates) ---
    UPROPERTY(BlueprintAssignable, Category = "3_Inventory|Events")
    FOnInventorySlotUpdated OnInventoryUpdated;

    UPROPERTY(BlueprintAssignable, Category = "3_Inventory|Events")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "3_Inventory|Events")
    FOnWeightChanged OnWeightChanged;

    UPROPERTY(BlueprintAssignable, Category = "3_Inventory|Events")
    FOnVolumeChanged OnVolumeChanged;

    // --- 4_Inventory|Settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4_Inventory|Settings")
    int32 MaxSlots = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4_Inventory|Settings")
    float MaxCarryWeight = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4_Inventory|Settings")
    float MaxCarryVolume = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4_Inventory|Settings")
    FGameplayTag InventoryBehaviorTag;

    UFUNCTION(BlueprintCallable, Category = "4_Inventory|Settings")
    virtual void SetMaxCarryWeight(float NewMaxWeight);

    UFUNCTION(BlueprintCallable, Category = "4_Inventory|Settings")
    virtual void SetMaxCarryVolume(float NewMaxVolume);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(ReplicatedUsing = OnRep_InventoryItems, BlueprintReadOnly, Category = "4_Inventory|Data")
    TArray<FInventoryItem> Items;

    UFUNCTION()
    void OnRep_InventoryItems();

    void NotifySlotChanged(int32 SlotIndex);
    void NotifyInventoryChanged();
    void UpdateItemIndexes();

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- 5_Inventory|RPCs ---
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerAddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(Client, Reliable)
    void ClientAddItemResponse(bool bSuccess);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRemoveItemByID(FGameplayTag ItemID, int32 Quantity);

    UFUNCTION(Client, Reliable)
    void ClientRemoveItemResponse(bool bSuccess);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerMoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPullItem(int32 FromIndex, UInventoryComponent* SourceInventory);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_TransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex);

    // Utility: Count occupied slots
    UFUNCTION(BlueprintCallable, Category="2_Inventory|Queries")
    virtual int32 GetNumOccupiedSlots() const;

    // Utility: Total quantity of a specific item (by ItemIDTag)
    UFUNCTION(BlueprintCallable, Category="2_Inventory|Queries")
    virtual int32 GetNumItemsOfType(FGameplayTag ItemID) const;

    // Utility: Swap two slots (can be useful for sorting, UI, etc)
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool SwapItems(int32 IndexA, int32 IndexB);

    // --- 5_Inventory|Container Restrictions ---
    /** Optional: Only allow storing items with these ItemIDs (empty = allow all) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="5_Inventory|Container")
    TArray<FGameplayTag> AllowedItemIDs;

    /** Returns true if this inventory accepts this ItemDataAsset */
    UFUNCTION(BlueprintCallable, Category="5_Inventory|Container")
    virtual bool CanAcceptItem(UItemDataAsset* ItemData) const;
};
