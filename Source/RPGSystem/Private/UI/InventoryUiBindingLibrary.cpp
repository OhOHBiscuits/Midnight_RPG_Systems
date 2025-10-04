#include "UI/InventoryUiBindingLibrary.h"
#include "UI/InventoryPanelWidget.h"
#include "Components/PanelWidget.h"
#include "Inventory/InventoryComponent.h"

bool UInventoryUiBindingLibrary::BindPanel(UInventoryPanelWidget* Panel, UPanelWidget* Container, UInventoryComponent* Inventory)
{
	if (!Panel || !Container || !Inventory) return false;
	Panel->SetSlotContainer(Container);
	Panel->InitializeWithInventory(Inventory);
	return true;
}
