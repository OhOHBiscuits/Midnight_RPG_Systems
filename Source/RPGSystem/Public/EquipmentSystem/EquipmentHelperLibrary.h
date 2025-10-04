// Source/RPGSystem/Public/EquipmentSystem/EquipmentHelperLibrary.h
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "EquipmentHelperLibrary.generated.h"

class UEquipmentComponent;
class UInventoryComponent;
class UWieldComponent;
class UItemDataAsset;
class AActor;
class APawn;
class APlayerController;
class APlayerState;

UCLASS()
class RPGSYSTEM_API UEquipmentHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// === BP-visible (keep original names / no overloads) ===
	UFUNCTION(BlueprintCallable, Category="1_Equipment-Helpers")
	static UEquipmentComponent* GetEquipmentPS(APlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Helpers")
	static UEquipmentComponent* GetEquipmentAny(AActor* OwnerLike);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Helpers")
	static UWieldComponent* GetWieldPS(AActor* OwnerLike);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Query")
	static UItemDataAsset* GetEquippedItemData(AActor* OwnerLike, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	static bool EquipBestFromInventoryIndex(
		APlayerState* PlayerState,
		UInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag PreferredSlot,
		bool bAlsoWield
	);

	// === C++-only helpers (no UFUNCTION to avoid UHT overload conflicts) ===
	static UEquipmentComponent* GetEquipmentPS(AActor* OwnerLike); // wrapper to ResolvePlayerState + GetEquipmentPS(PS)

	static bool EquipBestFromInventoryIndex(
		APawn* Pawn,
		UInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag PreferredSlot,
		bool bAlsoWield
	);

	static bool EquipBestFromInventoryIndex(
		AActor* OwnerLike,
		UInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag PreferredSlot,
		bool bAlsoWield
	);

private:
	static APlayerState* ResolvePlayerState(AActor* OwnerLike);
};
