// Source/RPGSystem/Public/EquipmentSystem/EquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EquipmentComponent.generated.h"

class UItemDataAsset;
class UInventoryComponent;

/** A single configurable equipment slot definition. */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FEquipmentSlotDef
{
	GENERATED_BODY()

	/** Friendly name for UI (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FName SlotName = NAME_None;

	/** Unique tag that identifies this slot (e.g., Slots.Weapon.Primary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTag SlotTag;

	/** Which items are allowed here. Empty query = allow anything. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTagQuery AllowedItemQuery;

	/** If false, systems may enforce that something must occupy this slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	bool bOptional = true;
};

/** Replicated record of what's in a slot (stored by ItemID tag). */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag SlotTag;

	/** ItemID of the equipped item; invalid = empty slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag ItemIDTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotCleared, FGameplayTag, SlotTag);

/**
 * Authoritative, replicated equipment list + queries and BP-friendly events.
 * Keeps UI updated via multicast (server) and OnRep (clients).
 */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	// ---------------- Config ----------------

	/** Weapon/tool slots (Primary, Secondary, SideArm, Melee, …). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Config")
	TArray<FEquipmentSlotDef> WeaponSlots;

	/** Armor/gear slots (Helmet, Chest, Hands, …). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Config")
	TArray<FEquipmentSlotDef> ArmorSlots;

	// ---------------- Replicated state ----------------

	/** The authoritative equipped list (by slot + ItemID). */
	UPROPERTY(ReplicatedUsing=OnRep_Equipped, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|State")
	TArray<FEquippedEntry> Equipped;

	// ---------------- UI events ----------------

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquipmentSlotCleared;

	// ---------------- Queries ----------------

	/** Returns the loaded item data in a slot (nullptr if empty). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	UItemDataAsset* GetEquippedItemData(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	bool IsEquipped(FGameplayTag SlotTag) const;

	/** Collects just the slot tags (weapons first, then armor). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	void GetAllSlotTags(TArray<FGameplayTag>& OutSlots) const;

	/** Pretty name for a slot: SlotName if set, otherwise from tag. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	FText GetSlotDisplayName(FGameplayTag Slot) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	void GetWeaponSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	void GetArmorSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const;

	// ---------------- Actions (client-safe; will run on server) ----------------

	/** Equip by consuming 1 item from an inventory index. */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex, bool bAutoApplyModifiers = true, bool bAlsoWield = false);

	/** Equip from an ItemID tag (does not consume inventory). */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool bAutoApplyModifiers = true, bool bAlsoWield = false);

	/** Clears the slot (no inventory destination). */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool UnequipSlot(FGameplayTag SlotTag);

	/** Clears the slot and tries to add 1 back into DestInventory. */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool UnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory);

	// ---------------- AActorComponent ----------------
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION() void OnRep_Equipped();

	// ---------------- RPCs ----------------
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex, bool bAutoApplyModifiers, bool bAlsoWield);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool bAutoApplyModifiers, bool bAlsoWield);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory);

private:
	// -------------- local helpers --------------
	const FEquipmentSlotDef* FindSlotDef(FGameplayTag SlotTag) const;
	int32 FindEquippedIndex(FGameplayTag SlotTag) const;
	bool IsItemAllowedInSlot(const FEquipmentSlotDef& SlotDef, const UItemDataAsset* ItemData) const;

	// Mutators (authority only)
	bool EquipLocal(FGameplayTag SlotTag, FGameplayTag ItemIDTag, bool bAutoApplyModifiers, bool bAlsoWield);
	bool UnequipLocal(FGameplayTag SlotTag);
	void NotifySlotSetLocal(FGameplayTag SlotTag, FGameplayTag ItemIDTag);
	void NotifySlotClearedLocal(FGameplayTag SlotTag);

	// Client-side diff cache for OnRep broadcasting
	TArray<FEquippedEntry> LocalLastEquippedCache;
};
