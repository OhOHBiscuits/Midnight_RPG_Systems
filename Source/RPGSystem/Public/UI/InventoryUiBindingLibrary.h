#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryUiBindingLibrary.generated.h"

class UInventoryPanelWidget;
class UPanelWidget;
class UInventoryComponent;

UCLASS()
class RPGSYSTEM_API UInventoryUiBindingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="1_Inventory-UI|Setup")
	static bool BindPanel(UInventoryPanelWidget* Panel, UPanelWidget* Container, UInventoryComponent* Inventory);
};
