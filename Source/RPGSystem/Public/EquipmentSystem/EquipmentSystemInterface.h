#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "EquipmentSystemInterface.generated.h"

// Forward declares (keep header light)
class UItemDataAsset;
class UInventoryComponent;
class UEquipmentComponent;
class UDynamicToolbarComponent;
class UWieldComponent;
class UAbilitySystemComponent;
class USkeletalMeshComponent;

UINTERFACE(BlueprintType)
class RPGSYSTEM_API UEquipmentSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement on PlayerState (accessors) and Pawn/Character (presentation).
 * - Presentation: attach visuals & push pose tags to AnimBP.
 * - Accessors: expose PlayerState-owned systems to UI/ViewModels.
 */
class RPGSYSTEM_API IEquipmentSystemInterface
{
	GENERATED_BODY()

public:
	// ---------------- Presentation ----------------
	// Attach the visual to a hand socket (draw / unholster).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Presentation")
	void Equip_AttachInHands(UItemDataAsset* ItemData, FName HandSocket, FGameplayTag PoseTag);

	// Attach the visual to a holster/body socket (put away).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Presentation")
	void Equip_AttachToHolster(UItemDataAsset* ItemData, FName HolsterSocket, FGameplayTag PoseTag);

	// Optional cleanup hook.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Presentation")
	void Equip_ClearAttachedItem();

	// Push a locomotion/upper-body pose tag into your AnimBP or movement system.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Presentation")
	void Equip_ApplyPoseTag(FGameplayTag PoseTag);

	// ---------------- Accessors ----------------
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	UInventoryComponent*      ES_GetInventory();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	UEquipmentComponent*      ES_GetEquipment();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	UDynamicToolbarComponent* ES_GetToolbar();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	UWieldComponent*          ES_GetWield();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	UAbilitySystemComponent*  ES_GetASC();

	// Main character mesh for attach points (hand/holster sockets).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="1_Equipment-Access")
	USkeletalMeshComponent*   ES_GetMainMesh();
};
