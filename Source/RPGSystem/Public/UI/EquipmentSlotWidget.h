// Source/RPGSystem/Public/UI/EquipmentSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EquipmentSlotWidget.generated.h"

class UInventoryDragDropOp;

/**
 * A simple “drop target” for equipment.
 * - You tell it which SlotTag it represents.
 * - When you drop an inventory item on it, it tries to equip:
 *     1) This SlotTag (if empty)
 *     2) If full and item has a Secondary in its data and that is empty → use that
 *     3) If both full and swapping is allowed → swap the old item into the source inventory
 * - Optional: auto-wield after a successful equip.
 */
UCLASS(Blueprintable)
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which equipment slot this widget represents, e.g. Tags.Slots.Weapon.Primary */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
	FGameplayTag SlotTag;

	/** Auto-wield the item after we equip it (holster logic lives in WieldComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bWieldAfterEquip = true;

	/** If the slot is full, are we allowed to swap the old item back into the source inventory? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bAllowSwapWhenFull = true;

protected:
	// We only need to handle drops; drag visuals are handled by the tile widget that started the drag.
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
};
