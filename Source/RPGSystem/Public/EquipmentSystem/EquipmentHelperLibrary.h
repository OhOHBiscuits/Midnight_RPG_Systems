#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "EquipmentHelperLibrary.generated.h"

class UEquipmentComponent;
class UInventoryComponent;
class UItemDataAsset;
class UWieldComponent;
class APlayerState;

UCLASS()
class RPGSYSTEM_API UEquipmentHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** PlayerState components – context can be Pawn, PS, or anything with access to it */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Resolve")
	static UEquipmentComponent* GetEquipmentPS(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Resolve")
	static UInventoryComponent* GetInventoryPS(AActor* ContextActor);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Resolve")
	static UWieldComponent* GetWieldPS(AActor* ContextActor);

	/** Convenience: calls EquipmentComponent->TryEquipByItemIDTag */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	static bool EquipByItemIDTag(AActor* ContextActor, const FGameplayTag& SlotTag, const FGameplayTag& ItemIDTag, bool bAlsoWield=false);

	/** Convenience: calls EquipmentComponent->TryUnequipSlotToInventory */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	static bool UnequipSlotToInventory(AActor* ContextActor, const FGameplayTag& SlotTag, UInventoryComponent* DestInventory);

	/** Try equip from a source inventory index choosing a good slot (Preferred first, then any allowed) */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	static bool EquipBestFromInventoryIndex(AActor* ContextActor, UInventoryComponent* SourceInventory, int32 SourceIndex, const FGameplayTag& PreferredSlot, bool bAlsoWield=false);

	/** Get item data currently in slot */
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Query")
	static UItemDataAsset* GetEquippedItemData(AActor* ContextActor, const FGameplayTag& SlotTag);
};
