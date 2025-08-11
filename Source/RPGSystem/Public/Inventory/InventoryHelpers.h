#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "InventoryHelpers.generated.h"

class UInventoryComponent;
class UItemDataAsset;

UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="1_Inventory|Helpers")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="1_Inventory|Helpers")
	static UItemDataAsset* FindItemDataByTag(UObject* WorldContextObject, const FGameplayTag& ItemID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="2_Inventory|Helpers")
	static bool ResolveItemPathByTag(const FGameplayTag& ItemID, FSoftObjectPath& OutPath);
};
