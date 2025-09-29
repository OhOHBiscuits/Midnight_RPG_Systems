#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "EquipmentHelperLibrary.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UDynamicToolbarComponent;
class UWieldComponent;
class UItemDataAsset;
class APlayerState;
class AActor;

UCLASS()
class RPGSYSTEM_API UEquipmentHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---------- Access (PlayerState-owned) ----------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Access")
	static APlayerState* GetPlayerStateFromActor(const AActor* Actor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Access")
	static UInventoryComponent* GetInventoryPS(const AActor* FromActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Access")
	static UEquipmentComponent* GetEquipmentPS(const AActor* FromActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Access")
	static UDynamicToolbarComponent* GetToolbarPS(const AActor* FromActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Access")
	static UWieldComponent* GetWieldPS(const AActor* FromActor);

	// ---------- Assets ----------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Assets")
	static UItemDataAsset* LoadItemDataByTag(FGameplayTag ItemIDTag, bool bSynchronousLoad);

	// ---------- Equipment (by tag) ----------
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipItemByTag(AActor* ContextActor, FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool UnequipSlot(AActor* ContextActor, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Queries")
	static UItemDataAsset* GetEquippedItemData(AActor* ContextActor, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Queries")
	static FGameplayTag GetEquippedItemIDTag(AActor* ContextActor, FGameplayTag SlotTag);

	// ---------- Inventory helpers ----------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory-Queries")
	static bool GetInventoryItemIDTag(UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag& OutItemIDTag, bool bLoadIfNeeded=true);

	// Equip clicked inventory item into Slot using its ItemIDTag (optional auto-wield)
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipFromInventoryIndexByItemIDTag(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag SlotTag, bool bAlsoWield=false, bool bLoadIfNeeded=true);

	// (You still have the index-based path too)
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipFromInventorySlot(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag SlotTag, bool bAlsoWield=false);

	// ---------- Ability bridge (TAG-ONLY) ----------
	// UI → GA: send ItemIDTag + SlotTag via tags (no pointers)
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Ability")
	static void SendEquipByItemIDEvent(AActor* AvatarActor, FGameplayTag EquipEventTag, FGameplayTag ItemIDTag, FGameplayTag SlotTag);

	// GA → parse tags back out (ItemIDTag from InstigatorTags, SlotTag from TargetTags)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Ability")
	static bool ParseEquipByItemIDEvent(const FGameplayEventData& EventData, FGameplayTag& OutItemIDTag, FGameplayTag& OutSlotTag);

	// ---------- Wield / Holster ----------
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Wield")
	static bool WieldEquippedSlot(AActor* ContextActor, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Wield")
	static bool ToggleHolster(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Wield")
	static bool Holster(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Wield")
	static bool Unholster(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Wield")
	static bool SetWieldProfile(AActor* ContextActor, FGameplayTag ProfileTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Wield")
	static bool IsHolstered(AActor* ContextActor);

	// ---------- Toolbar / Zones ----------
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Toolbar")
	static void ToolbarCycleNext(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Toolbar")
	static void ToolbarCyclePrev(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Toolbar")
	static void NotifyZoneTags(AActor* ContextActor, const FGameplayTagContainer& ZoneTags);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Toolbar")
	static void SetCombatState(AActor* ContextActor, bool bInCombat);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Toolbar")
	static TArray<FGameplayTag> GetToolbarItemIDs(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Toolbar")
	static int32 GetToolbarActiveIndex(AActor* ContextActor);

	// +++ New helpers (keep the rest of your file intact) +++

	/** From an ItemID tag, pick the best slot using PreferredEquipSlots in the data asset. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Queries")
	static bool DetermineBestEquipSlotForItemID(AActor* ContextActor, FGameplayTag ItemIDTag, FGameplayTag& OutSlotTag);

	/** Equip the clicked inventory item by ItemID preference:
	 *  - Primary if empty, else Secondary if empty, else swap (old -> source inventory). */
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipBestFromInventoryIndex(AActor* ContextActor, class UInventoryComponent* SourceInventory, int32 SourceIndex, bool bAlsoWield = false);

};
