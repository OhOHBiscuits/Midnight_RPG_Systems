#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EquipmentSlotWidget.generated.h"

class UInventoryDragDropOp;
class UImage; // forward-declare is fine for UPROPERTY pointers

/**
 * UI for one equipment slot (Primary/Secondary/Helmet/etc.).
 * - Lightweight: asks EquipmentComponent for current item and paints an icon.
 * - Handles drops from inventory tiles (smart-equip + optional swap).
 */
UCLASS(Blueprintable)
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which equipment slot this represents, e.g. Tags.Slots.Weapon.Primary */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
	FGameplayTag SlotTag;

	/** Auto-wield the item after we equip it (holster logic lives in WieldComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bWieldAfterEquip = true;

	/** If full, are we allowed to swap the old item back into the source inventory? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bAllowSwapWhenFull = true;

	/** Optional image widget to show the icon (bind in UMG if you want automatic icon) */
	UPROPERTY(meta=(BindWidgetOptional))
	UImage* IconImage = nullptr;

	/** Pull current item for SlotTag and paint the icon (async load safe). */
	UFUNCTION(BlueprintCallable, Category="1_Equipment-UI")
	void RefreshVisuals();

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeConstruct() override;
};
