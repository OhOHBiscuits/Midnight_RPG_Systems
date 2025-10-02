// Copyright ...
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;
class UWieldComponent;

/** Simple entry: which slot holds which ItemID tag */
USTRUCT(BlueprintType)
struct FEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ItemIDTag;

	FEquippedEntry() {}
	FEquippedEntry(const FGameplayTag& InSlot, const FGameplayTag& InItemID)
		: SlotTag(InSlot), ItemIDTag(InItemID) {}
};

/** Designer-facing slot definition */
USTRUCT(BlueprintType)
struct FEquipmentSlotDef
{
	GENERATED_BODY()

	/** Nice label for UI (e.g. "Primary", "Helmet") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FName SlotName = NAME_None;

	/** Unique tag for the slot (e.g. Slots.Weapon.Primary) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTag SlotTag;

	/** What items are allowed here (query against item’s tags) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTagQuery AllowedItemQuery;

	/** Whether this slot may be left empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	bool bOptional = true;
};

/** Events for UI and systems */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotCleared, FGameplayTag, SlotTag);

UCLASS(meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	// -------- Designer config --------
	/** Weapon/tool slots (Primary/Secondary/etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Config")
	TArray<FEquipmentSlotDef> WeaponSlots;

	/** Armor/gear slots (Head/Chest/etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Config")
	TArray<FEquipmentSlotDef> ArmorSlots;

	/** If true, we try to wield after equip when asked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Config")
	bool bAutoWieldAfterEquip = true;

	// -------- Runtime state (replicated) --------
	/** What’s currently equipped; replicated for UI on clients */
		/** Called whenever the Equipped array changes (any slot) */
	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	/** Called when a slot is cleared */
	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquipmentSlotCleared;

	/** Back-compat aliases for any listeners using older names */
	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquippedSlotCleared;

	// -------- Queries --------
	/** Returns display name for a slot tag, falling back to prettified tag */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Query")
	FText GetSlotDisplayName(const FGameplayTag& Slot) const;

	/** Get all slot tags (Weapons + Armor) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Query")
	void GetAllSlotTags(TArray<FGameplayTag>& Out) const;

	/** True if the slot currently has an item */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Query")
	bool IsSlotOccupied(const FGameplayTag& Slot) const;

	/** Returns the equipped Item Data (loaded) for a slot, or nullptr */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Query")
	UItemDataAsset* GetEquippedItemDataForSlot(const FGameplayTag& Slot) const;

	// -------- Actions (BP-callable, server-authoritative) --------
	/** Equip by inventory index (consumes 1 from SourceInventory) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByInventoryIndex(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield=false);

	/** Equip directly by ItemID tag (no inventory index) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByItemIDTag(const FGameplayTag& SlotTag, const FGameplayTag& ItemIDTag, bool bAlsoWield=false);

	/** Unequip to an inventory (player’s PS inventory if DestInventory null) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryUnequipSlotToInventory(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory=nullptr);

	/** Just clear the slot (drops equipment state; does not return to inv) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryUnequipSlot(const FGameplayTag& SlotTag);

	// Networking
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category="Equipment|Query", meta=(DisplayName="Get Equipped Item Data (compat)"))
	UItemDataAsset* GetEquippedItemData_Compat(const FGameplayTag& SlotTag) const;
protected:
	UFUNCTION()
	void OnRep_Equipped();
	
	UPROPERTY(ReplicatedUsing=OnRep_Equipped, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|State")
	TArray<FEquippedEntry> Equipped;
	// Server RPCs mirror BP methods (so BP calls on client still work)
	UFUNCTION(Server, Reliable)
	void Server_TryEquipByInventoryIndex(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield);

	UFUNCTION(Server, Reliable)
	void Server_TryEquipByItemIDTag(const FGameplayTag& SlotTag, FGameplayTag ItemIDTag, bool bAlsoWield);

	UFUNCTION(Server, Reliable)
	void Server_TryUnequipSlotToInventory(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory);

	UFUNCTION(Server, Reliable)
	void Server_TryUnequipSlot(const FGameplayTag& SlotTag);

private:
	/** Internal helpers */
	int32 FindEquippedIndex(const FGameplayTag& Slot) const;
	void SetSlot_Internal(const FGameplayTag& Slot, const FGameplayTag& ItemID);
	void ClearSlot_Internal(const FGameplayTag& Slot);

	/** Resolve or fallback to the player-state inventory for this owner */
	UInventoryComponent* FindPlayerStateInventory() const;

	/** True if item (by data) passes slot’s AllowedItemQuery */
	bool IsItemAllowedInSlot(const UItemDataAsset* ItemData, const FGameplayTag& Slot) const;

	/** Load item by ItemID tag via InventoryAssetManager/Helpers */
	UItemDataAsset* LoadItemDataByID(const FGameplayTag& ItemID) const;
};
