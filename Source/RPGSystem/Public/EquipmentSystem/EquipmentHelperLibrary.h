// Core/EquipmentHelperLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
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

	// ---------- Equipment (slots) ----------
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipItemByTag(AActor* ContextActor, FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool UnequipSlot(AActor* ContextActor, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Queries")
	static UItemDataAsset* GetEquippedItemData(AActor* ContextActor, FGameplayTag SlotTag);

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

	// Convenience reads for UI
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Toolbar")
	static TArray<FGameplayTag> GetToolbarItemIDs(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment-Toolbar")
	static int32 GetToolbarActiveIndex(AActor* ContextActor);
};
