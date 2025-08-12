#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryItem.h"
#include "ItemDataAsset.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.generated.h"

class AController;

//  UI/Event Hooks (unchanged)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdated, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, NewWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVolumeChanged, float, NewVolume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryFull, bool, bIsFull);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, const FInventoryItem&, Item, int32, QuantityAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FInventoryItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemTransferSuccess, const FInventoryItem&, Item);

//Access Model
UENUM(BlueprintType)
enum class EInventoryAccessMode : uint8
{
	Public     UMETA(DisplayName="Public"),
	ViewOnly   UMETA(DisplayName="ViewOnly"),
	Private    UMETA(DisplayName="Private")
};

USTRUCT(BlueprintType)
struct FInventoryAccessSettings
{
	GENERATED_BODY()

	/** Stable string id (Steam/Epic if available; fallback local GUID) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString OwnerStableId;

	/** Who can do what (server enforces) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInventoryAccessMode AccessMode = EInventoryAccessMode::Public;

	/** If true, party/ally rules apply for Public mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowAlliesAndParty = true;

	/** Optional: treat others as ViewOnly in Private areas */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOthersViewOnlyInPrivate = true;

	/** Optional: heirloom tag that bypasses private (for owner only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag HeirloomItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Heirloom"), /*ErrorIfNotFound*/false);
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	
	// Access / Permissions
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Inventory|Access")
	FInventoryAccessSettings Access;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Access")
	FGameplayTag InventoryAccessTag;

	bool UInventoryComponent::HasAccess_Public() const
	{
		return InventoryAccessTag == FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Public"));
	}

	bool UInventoryComponent::HasAccess_ViewOnly() const
	{
		return InventoryAccessTag == FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.ViewOnly"));
	}

	bool UInventoryComponent::HasAccess_Private() const
	{
		return InventoryAccessTag == FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Private"));
	}

	void UInventoryComponent::SetInventoryAccess(const FGameplayTag& NewAccessTag)
	{
		InventoryAccessTag = NewAccessTag;
	}

	/** Auto-initialize access mode from area the owner is in (Area.Base / Area.Player) */
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Access")
	void AutoInitializeAccessFromArea(AActor* ContextActor);

	/** Manually set owner id (otherwise resolved automatically from owner pawn) */
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Access")
	void SetOwnerStableId(const FString& NewId);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Access")
	void SetAccessMode(EInventoryAccessMode NewMode);

	// Permission checks (server authoritative)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access")
	bool CanView(AActor* Requestor) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Access")
	bool CanModify(AActor* Requestor) const;

	
	// Actions 
	
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool AddItem(UItemDataAsset* ItemData, int32 Quantity, AActor* Requestor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool RemoveItem(int32 SlotIndex, int32 Quantity, AActor* Requestor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool RemoveItemByID(FGameplayTag ItemID, int32 Quantity, AActor* Requestor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool MoveItem(int32 FromIndex, int32 ToIndex, AActor* Requestor = nullptr);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory, AActor* Requestor = nullptr);

	// Client helpers 
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

	// Split / Sort 
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool SplitStack(int32 SlotIndex, int32 SplitQuantity, AActor* Requestor = nullptr);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSplitStack(int32 SlotIndex, int32 SplitQuantity, AController* Requestor);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	virtual bool TrySplitStack(int32 SlotIndex, int32 SplitQuantity);

	// Sort
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Actions")
	void SortInventoryByName();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSortInventoryByName(AController* Requestor);

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
	void ServerSortInventoryByRarity(AController* Requestor);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSortInventoryByType(AController* Requestor);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSortInventoryByCategory(AController* Requestor);

	// Existing settings/queries/events (
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory|Type")
	FGameplayTag InventoryTypeTag;

	// Queries 
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

	// Settings 
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
	virtual bool SwapItems(int32 IndexA, int32 IndexB, AActor* Requestor = nullptr);

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

	//  Permission 
	AController* ResolveRequestorController(AActor* ExplicitRequestor) const;
	bool HeirloomBypassAllowed(AActor* Requestor, const FInventoryItem* OptionalItem = nullptr) const;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// RPCs 
	UFUNCTION(Server, Reliable, WithValidation) void ServerAddItem(UItemDataAsset* ItemData, int32 Quantity, AController* Requestor);
	UFUNCTION(Client, Reliable)               void ClientAddItemResponse(bool bSuccess);
	UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItem(int32 SlotIndex, int32 Quantity, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerRemoveItemByID(FGameplayTag ItemID, int32 Quantity, AController* Requestor);
	UFUNCTION(Client, Reliable)               void ClientRemoveItemResponse(bool bSuccess);
	UFUNCTION(Server, Reliable, WithValidation) void ServerMoveItem(int32 FromIndex, int32 ToIndex, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void ServerPullItem(int32 FromIndex, UInventoryComponent* SourceInventory, AController* Requestor);
	UFUNCTION(Server, Reliable, WithValidation) void Server_TransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex, AController* Requestor);
};
