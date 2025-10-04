//All rights Reserved Midnight Entertainment Studios LLC
// Source/RPGSystem/Public/Inventory/InventoryHelpers.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InventoryHelpers.generated.h"

class UUserWidget;
class UDataAsset;
class UItemDataAsset;
class UInventoryComponent;
class APlayerController;
class AController;
class AActor;

UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// -------- Controller / Component access --------
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Helpers|Access")
	static APlayerController* ResolvePlayerController(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Helpers|Access")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor);

	// -------- UI helpers --------
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Helpers")
	static bool CreateWidgetOnInteractor(AActor* Interactor, TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& OutWidget);

	// -------- Tag → Asset helpers (BP-visible) --------
	/** Resolve an ItemData asset by ItemIDTag (sync load by default). */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Helpers|Assets")
	static UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, FGameplayTag ItemIDTag, bool bSyncLoad /*= true*/);

	/** Resolve ANY DataAsset by tag (sync load by default). */
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Helpers|Assets", meta=(DeterminesOutputType="AssetClass"))
	static UDataAsset* LoadDataAssetByTag(UObject* WorldContextObject, FGameplayTag Tag, TSubclassOf<UDataAsset> AssetClass, bool bSyncLoad /*= true*/);

	UFUNCTION(BlueprintPure, Category="1_Inventory-Helpers|Assets")
	static bool ResolveDataAssetPathByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, FSoftObjectPath& OutPath);

	// -------- Compatibility helpers (safe stubs; wire later if needed) --------
	UFUNCTION(BlueprintCallable, Category="1_Inventory-Actions")
	static bool ClientRequestTransfer(AActor* Requestor, UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex /*= -1*/);

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Actions")
	static bool TryAddByItemIDTag(UInventoryComponent* Inventory, FGameplayTag ItemIDTag, int32 Quantity, int32& OutAddedIndex);

	// -------- C++-only convenience overloads (avoid UHT overload rules) --------
	static UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, FGameplayTag ItemIDTag) /* forwards to 3-arg */;

private:
	static APlayerController* ResolvePC_Internal(AActor* Actor);
	static AController*       ResolveController_Internal(AActor* Actor);
	static class APlayerState* ResolvePlayerState_Internal(AActor* Actor);
};
