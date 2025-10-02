// Source/RPGSystem/Public/UI/EquipmentSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "EquipmentSlotWidget.generated.h"

class UInventoryDragDropOp;
class UInventoryComponent;
class UEquipmentComponent;
class UImage;
class UItemDataAsset;

/** One equipment slot (Primary/Secondary/Helmet/etc.). */
UCLASS(Blueprintable)
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Which slot is this card representing? e.g. Slots.Weapon.Primary
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
	FGameplayTag SlotTag;

	// Optional root to filter drags (e.g., Slots.Weapon vs Slots.Armor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
	FGameplayTag AcceptRootTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bPreferThisSlotOnDrop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bWieldAfterEquip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bAllowSwapWhenFull = true;

	// Optional icon image to fill automatically; leave null to drive visuals fully in BP.
	UPROPERTY(meta=(BindWidgetOptional))
	UImage* IconImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment")
	bool bLoadAssetsIfNeeded = true;

	// Repaint from current equipped state
	UFUNCTION(BlueprintCallable, Category="1_Equipment-UI")
	void RefreshVisuals();

	// BP hooks so you can style and veto in blueprints
	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnRefreshVisualsBP(UItemDataAsset* ItemData);

	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnDragHoverChanged(bool bHovering);

	UFUNCTION(BlueprintNativeEvent, Category="1_Equipment-UI|Hooks")
	bool CanAcceptDrag_BP(UInventoryComponent* SourceInventory, int32 SourceIndex);
	virtual bool CanAcceptDrag_BP_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex) { return true; }

	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnDropOutcome(bool bSuccess, FGameplayTag UsedSlot);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;

	// Bound to component signals
	UFUNCTION() void HandleEquipChanged(FGameplayTag InSlot, UItemDataAsset* Item);
	UFUNCTION() void HandleSlotCleared(FGameplayTag InSlot);

private:
	// Cache the PS-owned EquipmentComponent we’re listening to
	TWeakObjectPtr<UEquipmentComponent> BoundEquip;

	// Hook up delegates
	void BindToEquipment();
};
