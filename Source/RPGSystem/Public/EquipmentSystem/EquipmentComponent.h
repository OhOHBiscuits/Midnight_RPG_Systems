// EquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;

USTRUCT(BlueprintType)
struct FEquipmentSlotDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FName SlotName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTag SlotTag; // e.g. Slots.Weapon.Primary

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTagQuery AllowedItemQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	bool bOptional = true;
};

USTRUCT(BlueprintType)
struct FEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag ItemIDTag;

	bool IsValid() const { return SlotTag.IsValid() && ItemIDTag.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotCleared, FGameplayTag, SlotTag);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentComponent();

	/** Replicated state: what’s currently equipped */
	UPROPERTY(ReplicatedUsing=OnRep_Equipped, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|State")
	TArray<FEquippedEntry> Equipped;

	/** Auto-apply modifiers on equip/unequip (server authoritative) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Config")
	bool bAutoApplyItemModifiers = true;

	/** Authored slot defs (Weapons / Armor) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Slots")
	TArray<FEquipmentSlotDef> WeaponSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment|Slots")
	TArray<FEquipmentSlotDef> ArmorSlots;

	/** Cached tag-only views built from the defs (for UI & helpers) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="1_Equipment|Slots")
	TArray<FGameplayTag> WeaponSlotTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="1_Equipment|Slots")
	TArray<FGameplayTag> ArmorSlotTags;

	// --- Events (kept both names for backward compat with existing widgets) ---
	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquipmentSlotCleared;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquippedSlotCleared;

	UFUNCTION()
	void OnRep_Equipped();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SlotSet(FGameplayTag SlotTag, UItemDataAsset* ItemData);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SlotCleared(FGameplayTag SlotTag);
	// --- multicast notifies so widgets update on all machines ---
	
	void Multicast_SlotSet_Implementation(FGameplayTag SlotTag, UItemDataAsset* ItemData);

	
	void Multicast_SlotCleared_Implementation(FGameplayTag SlotTag);

	/** Helper: local broadcast on the owner (server or client). */
	void LocalNotify_SlotSet(FGameplayTag SlotTag, UItemDataAsset* ItemData);
	void LocalNotify_SlotCleared(FGameplayTag SlotTag);

	// convenient local helpers so server paths stay tidy
	void NotifySlotSet(const FGameplayTag& SlotTag, UItemDataAsset* ItemData);
	void NotifySlotCleared(const FGameplayTag& SlotTag);
	// --- Slot queries (BP) ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	void GetWeaponSlotTags(TArray<FGameplayTag>& OutSlots) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	void GetArmorSlotTags(TArray<FGameplayTag>& OutSlots) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	void GetAllSlotTags(TArray<FGameplayTag>& OutSlots) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	FText GetSlotDisplayName(FGameplayTag Slot) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	void GetWeaponSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	void GetArmorSlotsWithNames(TArray<FGameplayTag>& OutTags, TArray<FText>& OutNames) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	bool GetSlotDefByTag(FGameplayTag Slot, FEquipmentSlotDef& OutDef) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Slots")
	bool DoesSlotAllowItem(FGameplayTag Slot, const FGameplayTagContainer& ItemTags) const;

	// --- Equip state queries ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	UItemDataAsset* GetEquippedItemData(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	bool IsEquipped(FGameplayTag SlotTag) const;

	// --- Actions (server-authoritative; Try* wrappers auto-RPC) ---
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool UnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryUnequipSlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryUnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory);

	// GAS hook
	UFUNCTION(BlueprintNativeEvent, Category="1_Equipment|Modifiers")
	void ApplyItemModifiers(UItemDataAsset* ItemData, bool bApply);
	virtual void ApplyItemModifiers_Implementation(UItemDataAsset* ItemData, bool bApply);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void RebuildSlotTagCaches();
	const FEquipmentSlotDef* FindSlotDef(FGameplayTag SlotTag) const;
	bool IsItemAllowedInSlot(const FEquipmentSlotDef& SlotDef, UItemDataAsset* ItemData) const;
	int32 FindEquippedIndex(FGameplayTag SlotTag) const;

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void ServerEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);
	UFUNCTION(Server, Reliable, WithValidation) void ServerEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);
	UFUNCTION(Server, Reliable, WithValidation) void ServerUnequipSlot(FGameplayTag SlotTag);
	UFUNCTION(Server, Reliable) void Server_UnequipSlotToInventory(FGameplayTag SlotTag, UInventoryComponent* DestInventory, class AController* Requestor);
};
