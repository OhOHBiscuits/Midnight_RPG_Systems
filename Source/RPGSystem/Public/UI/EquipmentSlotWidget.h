//All rights Reserved Midnight Entertainment Studios LLC
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EquipmentSlotWidget.generated.h"

class UEquipmentComponent;
class UItemDataAsset;

/**
 * Tracks one equipment slot and caches ItemIDTag + ItemData.
 * Safe BP setters keep both in sync and fire UI events.
 * NOTE: This widget is UI-only; it does not equip/unequip on the component.
 */
UCLASS(BlueprintType, Blueprintable)
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which slot this widget represents (e.g., Slots.Weapon.Primary) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Equipment")
	FGameplayTag SlotTag;

	/** Cached item data for this slot (may be null when empty) */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-Equipment")
	TObjectPtr<UItemDataAsset> CurrentItemData = nullptr;

	/** Cached lightweight ID tag for the equipped item (Invalid when empty) */
	UPROPERTY(BlueprintReadWrite, Category="1_Inventory-Equipment")
	FGameplayTag CurrentItemIDTag;

	/** When setting the tag via BP setters, automatically try to resolve/load the data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Equipment")
	bool bResolveDataOnTagSet = true;

	/** Manually set the EquipmentComponent (unbinds previous, binds new, refreshes) */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void SetEquipmentComponent(UEquipmentComponent* InEquipment);

	/** Change the slot this widget watches and refresh */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void SetSlotTagAndRefresh(FGameplayTag NewSlotTag);

	/** Pull state from the component once (safe to call anytime) */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void RefreshFromComponent();

	/** Force a re-sync from the EquipmentComponent (alias for RefreshFromComponent) */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void ForceSyncFromEquipment() { RefreshFromComponent(); }

	/** UI hook — implement in BP to update icon, text, etc. */
	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-Equipment")
	void BP_OnSlotUpdated(UItemDataAsset* NewItemData);

	/** UI hook — fires whenever the cached ItemIDTag changes (equip/unequip). */
	UFUNCTION(BlueprintImplementableEvent, Category="1_Inventory-Equipment")
	void BP_OnSlotItemIDTagUpdated(const FGameplayTag& NewItemIDTag);

	/** UI-only setters (do NOT modify EquipmentComponent). Useful for BP hotfixes/overrides. */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void BP_SetCurrentItemData(UItemDataAsset* InData, bool bFireEvents = true);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Equipment")
	void BP_SetCurrentItemIDTag(FGameplayTag InTag, bool bResolveData = true, bool bFireEvents = true);

	/** Convenience */
	UFUNCTION(BlueprintPure, Category="1_Inventory-Equipment")
	UEquipmentComponent* GetEquipmentComponent() const { return Equipment; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEquipmentComponent> Equipment = nullptr;

	// Event handlers
	UFUNCTION()
	void HandleEquipChanged(FGameplayTag ChangedSlot, UItemDataAsset* NewData);

	UFUNCTION()
	void HandleSlotCleared(FGameplayTag ClearedSlot);

	// Helpers
	UEquipmentComponent* AutoResolveEquipment() const;
	void BindToEquipment(UEquipmentComponent* InEquipment);
	void UnbindFromEquipment();

	/** Pull ItemIDTag + ItemData for SlotTag from Equipment; loads by tag if needed. */
	void UpdateCacheFromEquipment();

	/** Internal single place to broadcast after cache changes (de-duped) */
	void BroadcastCache();
};
