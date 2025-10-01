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

/**
 * One equipment slot (Primary/Secondary/Helmet/etc.).
 * Listens to the equipment component and repaints itself.
 */
UCLASS(Blueprintable)
class RPGSYSTEM_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bPreferThisSlotOnDrop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bWieldAfterEquip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup")
	bool bAllowSwapWhenFull = true;

	UPROPERTY(meta=(BindWidgetOptional))
	UImage* IconImage = nullptr;
	
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Equipment")
    bool bLoadAssetsIfNeeded = true;  

	/** Pull current item and paint just what we need (icon/name via BP). */
	UFUNCTION(BlueprintCallable, Category="1_Equipment-UI")
	void RefreshVisuals();

	// ——— BP hooks to let you style/guard without C++ edits ———
	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnRefreshVisualsBP(UItemDataAsset* ItemData);

	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnDragHoverChanged(bool bHovering);

	UFUNCTION(BlueprintNativeEvent, Category="1_Equipment-UI|Hooks")
	bool CanAcceptDrag_BP(UInventoryComponent* SourceInventory, int32 SourceIndex);
	virtual bool CanAcceptDrag_BP_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex) { return true; }

	UFUNCTION(BlueprintImplementableEvent, Category="1_Equipment-UI|Hooks")
	void OnDropOutcome(bool bSuccess, FGameplayTag UsedSlot);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment-UI|Setup", meta=(ExposeOnSpawn="true"))
    FGameplayTag AcceptRootTag;

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InEvent, UDragDropOperation* InOp) override;
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION() void HandleEquipChanged(FGameplayTag InSlot, UItemDataAsset* Item);
	UFUNCTION() void HandleSlotCleared(FGameplayTag InSlot);

	
	/** Bind to the equipment component on our owning pawn. */
	void BindToEquipment();

	/** Your existing delegates: equip changed (with data) & slot cleared. */
	UFUNCTION(BlueprintPure, Category="1_Equipment|Resolve")
	static class APlayerState* ResolvePlayerState(AActor* ContextActor);
	UFUNCTION() void HandleEquipCleared(FGameplayTag ClearedSlot);

private:
	TWeakObjectPtr<UEquipmentComponent> CachedEquip;
};
