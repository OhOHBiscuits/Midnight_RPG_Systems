// InventoryHelpers.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InventoryHelpers.generated.h"

class UItemDataAsset;
class UInventoryComponent;
class APlayerController;
class AController;
class AActor;

/**
 * Inventory helpers: safe lookups used by BP & C++.
 */
UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get the first InventoryComponent found on an Actor (Controller, Pawn, PlayerState, or the Actor itself). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor);

	/** Convenience: try to resolve an InventoryComponent from ANY object (Actor, Component, etc.). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static UInventoryComponent* GetInventoryComponentFromObject(const UObject* Object);

	/** Resolve the local PlayerController from an Object (Actor/Controller/world context). Returns nullptr if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static APlayerController* GetPlayerControllerFromObject(const UObject* Object);

	/** Resolve the local PlayerController from an Actor. Returns nullptr if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static APlayerController* GetPlayerControllerFromActor(AActor* Actor);

	/** Net owner PC (tries Interactor first, falls back to world index 0). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static APlayerController* GetNetOwningPlayerController(const UObject* WorldContextObject, AActor* Interactor);

	/** Find an ItemData by ItemID tag via AssetManager (cooked-safe). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Assets", meta=(WorldContext="WorldContextObject"))
	static UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, FGameplayTag ItemIdTag);

	// **NEW**: Used by StorageActor to open UI on the *owning player's* viewport.
	UFUNCTION(BlueprintCallable, Category="1_Inventory|UI", meta=(WorldContext="Interactor"))
	static UUserWidget* CreateWidgetOnInteractor(AActor* Interactor, TSubclassOf<UUserWidget> WidgetClass, bool& bIsLocalPlayer);

	
	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static APlayerController* ResolvePlayerController(AActor* Actor);
	
	
	
};
