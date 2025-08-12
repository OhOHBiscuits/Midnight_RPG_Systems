#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InventoryHelpers.generated.h"

class UInventoryComponent;

UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Helpers")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor);

	// NEW: stable player id (works without Steam/Epic)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static FString GetStablePlayerId(AActor* Actor);

	// NEW: party/ally stubs (permissive for now)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static bool IsSameParty(AActor* A, AActor* B);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static bool IsAlly(AActor* A, AActor* B);

	// NEW: find area tag at actor location (Area.Base / Area.Player)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Helpers")
	static bool FindOverlappingAreaTag(AActor* Actor, FGameplayTag& OutAreaTag);

	// (existing)
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Helpers")
	static class UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, const FGameplayTag& ItemID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="2_Inventory|Helpers")
	static bool ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath);
};
