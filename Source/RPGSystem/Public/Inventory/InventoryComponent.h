// InventoryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "InventoryItem.h"
#include "ItemDataAsset.h"
#include "InventoryComponent.generated.h"

class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeChanged, float, NewVolume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFull, bool, bIsFull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, const FInventoryItem&, Item, int32, QuantityAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemTransferSuccess, const FInventoryItem&, Item);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// Access
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_AccessTag, Category="1_Inventory|Access")
	FGameplayTag AccessTag;

	UFUNCTION() void OnRep_AccessTag();
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Access") void SetInventoryAccess(const FGameplayTag& NewAccessTag);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") FGameplayTag GetInventoryAccessTag() const { return AccessTag; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") bool HasAccess_Public() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") bool HasAccess_ViewOnly() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") bool HasAccess_Private() const;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Access") void AutoInitializeAccessFromArea(AActor* ContextActor);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") bool CanView(AActor* Requestor) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access") bool CanModify(AActor* Requestor) const;

	// Core actions (server-auth; clients use Try*)
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool AddItem(UItemDataAsset* ItemData, int32 Quantity, AActor* Requestor = nullptr);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool RemoveItem(int32 SlotIndex, int32 Quantity, AActor* Requestor = nullptr);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool RemoveItemByID(FGameplayTag ItemID, int32 Quantity, AActor* Requestor = nullptr);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool MoveItem(int32 FromIndex, int32 ToIndex, AActor* Requestor = nullptr);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory, AActor* Requestor = nullptr);

	// Client helpers (auto-RPC)
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TryAddItem(UItemDataAsset* ItemData, int32 Quantity);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TryRemoveItem(int32 SlotIndex, int32 Quantity);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TryMoveItem(int32 FromIndex, int32 ToIndex);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory);

	// Split stack
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool SplitStack(int32 SlotIndex, int32 SplitQuantity, AActor* Requestor = nullptr);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSplitStack(int32 SlotIndex, int32 SplitQuantity, AController* Requestor);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool TrySplitStack(int32 SlotIndex, int32 SplitQuantity);

	// Cross-inventory convenience
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") bool PushToInventory(UInventoryComponent* TargetInventory, int32 FromIndex, int32 TargetIndex = -1);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") bool PullFromInventory(UInventoryComponent* SourceInventory, int32 SourceIndex, int32 TargetIndex = -1);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") bool RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex = -1);

	// Sorting
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void SortInventoryByName();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSortInventoryByName(AController* Requestor);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void RequestSortInventoryByName();

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void SortInventoryByRarity();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSortInventoryByRarity(AController* Requestor);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void RequestSortInventoryByRarity();

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void SortInventoryByType();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSortInventoryByType(AController* Requestor);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void RequestSortInventoryByType();

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void SortInventoryByCategory();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSortInventoryByCategory(AController* Requestor);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Sort") void RequestSortInventoryByCategory();

	// Queries
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual bool IsInventoryFull() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual float GetCurrentWeight() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual float GetCurrentVolume() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual FInventoryItem GetItem(int32 SlotIndex) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual int32 FindFreeSlot() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual int32 FindStackableSlot(UItemDataAsset* ItemData) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual int32 FindSlotWithItemID(FGameplayTag ItemID) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") virtual FInventoryItem GetItemByID(FGameplayTag ItemID) const;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Queries") virtual int32 GetNumOccupiedSlots() const;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Queries") virtual int32 GetNumItemsOfType(FGameplayTag ItemID) const;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|UI") int32 GetNumUISlots() const;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|UI") void GetUISlotInfo(TArray<int32>& OutSlotIndices, TArray<UItemDataAsset*>& OutItemData, TArray<int32>& OutQuantities) const;

	// NOTE: Returning containers by ref isn’t BP-friendly; keep for C++ only.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries")
	const TArray<FInventoryItem>& GetItems() const { return Items; }
	const TArray<FInventoryItem>& GetItem() const { return Items; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") bool IsEmpty() const { return GetNumOccupiedSlots() == 0; }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") bool IsNotEmpty() const { return GetNumOccupiedSlots() > 0; }

	// Filters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") TArray<FInventoryItem> FilterItemsByRarity(FGameplayTag RarityTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") TArray<FInventoryItem> FilterItemsByCategory(FGameplayTag CategoryTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") TArray<FInventoryItem> FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") TArray<FInventoryItem> FilterItemsByType(FGameplayTag TypeTag) const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Queries") TArray<FInventoryItem> FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll=false) const;

	// Misc
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions") virtual bool SwapItems(int32 IndexA, int32 IndexB, AActor* Requestor = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Container")
	TArray<FGameplayTag> AllowedItemIDs;

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Container")
	virtual bool CanAcceptItem(UItemDataAsset* ItemData) const;
	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") int32 MaxSlots = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") float MaxCarryWeight = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") float MaxCarryVolume = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") FGameplayTag InventoryTypeTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") FGameplayTag InventoryBehaviorTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Settings") bool bAutoSort = false;
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings") virtual void SetMaxCarryWeight(float NewMaxWeight);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings") virtual void SetMaxCarryVolume(float NewMaxVolume);
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Settings") void SetMaxSlots(int32 NewMaxSlots);
	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory|State") float CurrentWeight = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory|State") float CurrentVolume = 0.f;

	// Events
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnInventorySlotUpdated OnInventoryUpdated;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnInventoryChanged OnInventoryChanged;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnWeightChanged OnWeightChanged;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnVolumeChanged OnVolumeChanged;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnInventoryFull OnInventoryFull;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnItemAdded OnItemAdded;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnItemRemoved OnItemRemoved;
	UPROPERTY(BlueprintAssignable, Category="1_Inventory|Events") FOnItemTransferSuccess OnItemTransferSuccess;

	UPROPERTY(Transient)
	TArray<FInventoryItem> ClientPrevItems;	
	static bool ItemsEqual_Client(const FInventoryItem& A, const FInventoryItem& B);

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing=OnRep_InventoryItems, BlueprintReadOnly, Category="1_Inventory|Data")
	TArray<FInventoryItem> Items;

	UFUNCTION() void OnRep_InventoryItems();

	void NotifySlotChanged(int32 SlotIndex);
	void NotifyInventoryChanged();
	void UpdateItemIndexes();
	void AdjustSlotCountIfNeeded();
	AController* ResolveRequestorController(AActor* ExplicitRequestor) const;
	void RecalculateWeightAndVolume();

	bool bWasFull = false;

public:
	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void ServerAddItem(UItemDataAsset* ItemData, int32 Quantity, AController* Requestor);
	UFUNCTION(Client, Reliable) void ClientAddItemResponse(bool bSuccess);
	UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItem(int32 SlotIndex, int32 Quantity, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItemByID(FGameplayTag ItemID, int32 Quantity, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerMoveItem(int32 FromIndex, int32 ToIndex, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerPullItem(int32 FromIndex, UInventoryComponent* SourceInventory, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void Server_TransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex, AController* Requestor);
};
