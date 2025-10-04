//All rights Reserved Midnight Entertainment Studios LLC
// Source/RPGSystem/Public/EquipmentSystem/EquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EquipmentComponent.generated.h"

class UItemDataAsset;
class UInventoryComponent;

/** Authoring-time definition for a single equipment slot */
USTRUCT(BlueprintType)
struct RPGSYSTEM_API FEquipmentSlotDef
{
	GENERATED_BODY()

	/** Unique slot id (e.g., Slots.Weapon.Primary, Slots.Armor.Head) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot")
	FGameplayTag SlotTag;

	/** UI-friendly name for this slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Slot")
	FText SlotName;

	/** Optional: default item to place here (by ID tag) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default")
	FGameplayTag ItemIDTag;

	/** Optional: default item to place here (soft fallback) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Default")
	TSoftObjectPtr<UItemDataAsset> ItemData;

	/** Optional: root filter; item must have a tag matching this root to be valid for the slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Filter")
	FGameplayTag AcceptRootTag;
};

/** Replicated state: what is currently equipped in a slot */
USTRUCT(BlueprintType)
struct FEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag ItemIDTag; // mirror for lightweight saves

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UItemDataAsset> ItemData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentChangedSignature, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipmentSlotClearedSignature, FGameplayTag, SlotTag);

UCLASS(ClassGroup=(RPG), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	// --- Slot sets you author per character/class (BP-editable, not replicated) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment-Slots|Weapons")
	TArray<FEquipmentSlotDef> WeaponSlotDefs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment-Slots|Armor")
	TArray<FEquipmentSlotDef>  ArmorSlotDefs;

	// Concatenate both arrays (for UI or logic)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Slots|Query")
	void GetAllSlotDefs(TArray<FEquipmentSlotDef>& OutDefs) const;

	// Find a specific slot definition
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Slots|Query")
	bool GetSlotDef(const FGameplayTag& SlotTag, FEquipmentSlotDef& OutDef) const;

	// Optional: server helper to prefill equipment from slot defs (uses ItemIDTag first, then ItemData)
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Slots|Init")
	void InitializeSlotsFromDefinitions(bool bOnlyIfEmpty = true);

	// --- Replication: equipped state ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_Equipped, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment-State")
	TArray<FEquippedEntry> Equipped;

	UFUNCTION()
	void OnRep_Equipped();

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category="1_Equipment-Events")
	FEquipmentChangedSignature OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment-Events")
	FEquipmentSlotClearedSignature OnEquipmentSlotCleared;

	// Back-compat aliases
	UPROPERTY(BlueprintAssignable, Category="1_Equipment-Events")
	FEquipmentChangedSignature OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment-Events")
	FEquipmentSlotClearedSignature OnEquippedSlotCleared;

	// --- Queries ---
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Query")
	UItemDataAsset* GetEquippedItemData(const FGameplayTag& SlotTag) const;

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Query")
	bool IsSlotOccupied(const FGameplayTag& SlotTag) const;

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Query")
	void GetAllEquipped(TArray<FEquippedEntry>& Out) const { Out = Equipped; }

	// --- Actions (client → server) ---
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	bool TryEquipByInventoryIndex(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	bool TryUnequipSlotToInventory(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory);

private:
	// Lookup
	const FEquipmentSlotDef* FindSlotDef(const FGameplayTag& SlotTag) const;

	// Replicated state helpers
	FEquippedEntry*       FindOrAddEntry(const FGameplayTag& SlotTag);
	const FEquippedEntry* FindEntry(const FGameplayTag& SlotTag) const;
	bool                  RemoveEntry(const FGameplayTag& SlotTag);
	UItemDataAsset*       ResolveData(const FEquippedEntry& E) const;

	// Validation
	bool ValidateItemForSlot(const FGameplayTag& SlotTag, const UItemDataAsset* Data) const;

	// Server logic
	bool Equip_Internal(const FGameplayTag& SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex);
	bool Unequip_Internal(const FGameplayTag& SlotTag, UInventoryComponent* DestInventory);

	// RPCs (implement *_Implementation in .cpp)
	UFUNCTION(Server, Reliable)
	void Server_TryEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* SourceInventory, int32 SourceIndex);

	UFUNCTION(Server, Reliable)
	void Server_TryUnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory);
};
