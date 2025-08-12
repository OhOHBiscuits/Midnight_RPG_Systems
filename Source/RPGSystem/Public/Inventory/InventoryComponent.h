#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryItem.h"
#include "ItemDataAsset.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.generated.h"

// --- UI/Event Hooks ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeChanged, float, NewVolume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFull, bool, bIsFull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, const FInventoryItem&, Item, int32, QuantityAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemTransferSuccess, const FInventoryItem&, Item);

UENUM(BlueprintType)
enum class EInventoryAccessIntent : uint8
{
    View,
    Add,       // deposit
    Remove,    // withdraw
    Manage     // move/sort/transfer/settings
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    // 1_Inventory|Actions
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool AddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool RemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool RemoveItemByID(FGameplayTag ItemID, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool MoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TryAddItem(UItemDataAsset* ItemData, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TryRemoveItem(int32 SlotIndex, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TryMoveItem(int32 FromIndex, int32 ToIndex);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex);

    // Split
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool SplitStack(int32 SlotIndex, int32 SplitQuantity);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSplitStack(int32 SlotIndex, int32 SplitQuantity);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool TrySplitStack(int32 SlotIndex, int32 SplitQuantity);

    // Sort
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    void SortInventoryByName();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSortInventoryByName();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    void RequestSortInventoryByName();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void RequestSortInventoryByRarity();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void RequestSortInventoryByType();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void RequestSortInventoryByCategory();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void SortInventoryByRarity();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void SortInventoryByType();

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort")
    void SortInventoryByCategory();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSortInventoryByRarity();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSortInventoryByType();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSortInventoryByCategory();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Type")
    FGameplayTag InventoryTypeTag;

    // --- Access control (header) ---
    

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Privacy, Category="1_Inventory|Access")
    FGameplayTag PrivacyTag; // e.g., Inventory.Privacy.Public / Team / OwnerOnly / Locked

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access")
    bool CanActorAccess(AActor* Actor, EInventoryAccessIntent Intent) const;

    UFUNCTION()
    void OnRep_Privacy();

    UFUNCTION()
    void OnRep_Owner(); // If this was declared, it must exist even if it just notifies UI


    // 1_Inventory|Queries
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual bool IsInventoryFull() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual float GetCurrentWeight() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual float GetCurrentVolume() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual FInventoryItem GetItem(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual int32 FindFreeSlot() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual int32 FindStackableSlot(UItemDataAsset* ItemData) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual int32 FindSlotWithItemID(FGameplayTag ItemID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    virtual FInventoryItem GetItemByID(FGameplayTag ItemID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    const TArray<FInventoryItem>& GetAllItems() const { return Items; }

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Queries")
    virtual int32 GetNumOccupiedSlots() const;

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Queries")
    virtual int32 GetNumItemsOfType(FGameplayTag ItemID) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnInventorySlotUpdated OnInventoryUpdated;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnWeightChanged OnWeightChanged;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnVolumeChanged OnVolumeChanged;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnInventoryFull OnInventoryFull;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnItemAdded OnItemAdded;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnItemRemoved OnItemRemoved;

    UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events")
    FOnItemTransferSuccess OnItemTransferSuccess;

    // 1_Inventory|Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings")
    int32 MaxSlots = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings")
    float MaxCarryWeight = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings")
    float MaxCarryVolume = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings")
    FGameplayTag InventoryBehaviorTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings")
    bool bAutoSort = false;

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings")
    virtual void SetMaxCarryWeight(float NewMaxWeight);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings")
    virtual void SetMaxCarryVolume(float NewMaxVolume);

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings")
    void SetMaxSlots(int32 NewMaxSlots);

    // Filters
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    TArray<FInventoryItem> FilterItemsByRarity(FGameplayTag RarityTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    TArray<FInventoryItem> FilterItemsByCategory(FGameplayTag CategoryTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    TArray<FInventoryItem> FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    TArray<FInventoryItem> FilterItemsByType(FGameplayTag TypeTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
    TArray<FInventoryItem> FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll=false) const;

    // Misc
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
    virtual bool SwapItems(int32 IndexA, int32 IndexB);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Container")
    TArray<FGameplayTag> AllowedItemIDs;

    UFUNCTION(BlueprintCallable, Category="1_Inventory|Container")
    virtual bool CanAcceptItem(UItemDataAsset* ItemData) const;

    UFUNCTION(BlueprintCallable, Category="1_Inventory")
    int32 GetNumUISlots() const;

    UFUNCTION(BlueprintCallable, Category="1_Inventory")
    void GetUISlotInfo(TArray<int32>& OutSlotIndices, TArray<UItemDataAsset*>& OutItemData, TArray<int32>& OutQuantities) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory")
    bool IsSlotLimited() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory")
    bool IsWeightLimited() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory")
    bool IsVolumeLimited() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory")
    bool IsUnlimited() const;

    FORCEINLINE const TArray<FInventoryItem>& GetItems() const { return Items; }

    // --- Ownership (Header) ---
    UFUNCTION(BlueprintCallable, Category="1_Inventory|Ownership")
    void SetInventoryOwnerId(const FString& NewOwnerId); // BP/one-arg entry

    // Internal helper (no UFUNCTION, different name so no ambiguity)
    void SetInventoryOwnerIdInternal(const FString& NewOwnerId, bool bLock);

    // Data
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Ownership")
    FString InventoryOwnerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Ownership")
    bool bOwnerLocked = true;

    // Optional convenience (inline is fine)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Ownership")
    bool IsOwnerSafeContainer() const { return bOwnerLocked; }
   // --- Ownership & Privacy -----------------------------------------------------

/** Lock toggled by owner/admin (e.g., while crafting or during a raid). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Privacy, Category="0_Inventory|Ownership")
FGameplayTag PrivacyTag = FGameplayTag::RequestGameplayTag(FName("Inventory.Privacy.Public"));

/** Optional allow-list of player IDs (string IDs) that can access when private. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Owner, Category="0_Inventory|Ownership")
TArray<FString> AllowedUserIds;

/** Optional “groups” the owner assigns (party/tribe/etc). You can keep these as
    strings or gameplay tags depending on your wider systems. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Owner, Category="0_Inventory|Ownership")
TArray<FString> AllowedGroupIds;

/** If true, only owner/admin may change owner/privacy. */

UFUNCTION(BlueprintCallable, BlueprintPure, Category="0_Inventory|Ownership")
FString GetInventoryOwnerId() const { return InventoryOwnerId; }

UFUNCTION(BlueprintCallable, Category="0_Inventory|Ownership")
void SetPrivacyTag(FGameplayTag NewPrivacy);

UFUNCTION(BlueprintCallable, BlueprintPure, Category="0_Inventory|Ownership")
FGameplayTag GetPrivacyTag() const { return PrivacyTag; }

UFUNCTION(BlueprintCallable, Category="0_Inventory|Ownership")
void AddAllowedUserId(const FString& UserId);

UFUNCTION(BlueprintCallable, Category="0_Inventory|Ownership")
void RemoveAllowedUserId(const FString& UserId);

// Core guard for UI/open/transfer/manage calls
UFUNCTION(BlueprintCallable, BlueprintPure, Category="0_Inventory|Ownership")
bool CanActorAccess(AActor* Querier, EInventoryAccessIntent Intent) const;

// RepNotifies
UFUNCTION() void OnRep_Owner();
UFUNCTION() void OnRep_Privacy();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(ReplicatedUsing=OnRep_InventoryItems, BlueprintReadOnly, Category="1_Inventory|Data")
    TArray<FInventoryItem> Items;

    UFUNCTION()
    void OnRep_InventoryItems();
    void NotifySlotChanged(int32 SlotIndex);
    void NotifyInventoryChanged();
    void UpdateItemIndexes();
    void AdjustSlotCountIfNeeded();
    
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // RPCs
    UFUNCTION(Server, Reliable, WithValidation) void ServerAddItem(UItemDataAsset* ItemData, int32 Quantity);
    UFUNCTION(Client, Reliable)               void ClientAddItemResponse(bool bSuccess);
    UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItem(int32 SlotIndex, int32 Quantity);
    UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItemByID(FGameplayTag ItemID, int32 Quantity);
    UFUNCTION(Client, Reliable)               void ClientRemoveItemResponse(bool bSuccess);
    UFUNCTION(Server, Reliable, WithValidation) void ServerMoveItem(int32 FromIndex, int32 ToIndex);
    UFUNCTION(Server, Reliable, WithValidation) void ServerTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);
    UFUNCTION(Server, Reliable, WithValidation) void ServerPullItem(int32 FromIndex, UInventoryComponent* SourceInventory);
    UFUNCTION(Server, Reliable, WithValidation) void Server_TransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex);
};
