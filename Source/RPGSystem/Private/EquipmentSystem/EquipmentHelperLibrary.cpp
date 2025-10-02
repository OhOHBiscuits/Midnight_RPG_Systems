#include "EquipmentSystem/EquipmentHelperLibrary.h"
#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;

	// Prefer PlayerState (your setup keeps Inventory + Equipment there)
	if (APawn* P = Cast<APawn>(ContextActor))
	{
		if (APlayerState* PS = P->GetPlayerState())
		{
			if (UEquipmentComponent* C = PS->FindComponentByClass<UEquipmentComponent>())
				return C;
		}
	}

	// Fallback: look on the owner directly
	return ContextActor->FindComponentByClass<UEquipmentComponent>();
}

UInventoryComponent* UEquipmentHelperLibrary::GetInventoryPS(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;

	if (APawn* P = Cast<APawn>(ContextActor))
	{
		if (APlayerState* PS = P->GetPlayerState())
		{
			if (UInventoryComponent* C = PS->FindComponentByClass<UInventoryComponent>())
				return C;
		}
	}

	return ContextActor->FindComponentByClass<UInventoryComponent>();
}

UWieldComponent* UEquipmentHelperLibrary::GetWieldPS(AActor* ContextActor)
{
	if (!ContextActor) return nullptr;

	if (APawn* P = Cast<APawn>(ContextActor))
	{
		if (APlayerState* PS = P->GetPlayerState())
		{
			if (UWieldComponent* C = PS->FindComponentByClass<UWieldComponent>())
				return C;
		}
	}

	return ContextActor->FindComponentByClass<UWieldComponent>();
}

UItemDataAsset* UEquipmentHelperLibrary::GetEquippedItemData(AActor* ContextActor, const FGameplayTag& SlotTag)
{
	if (UEquipmentComponent* E = GetEquipmentPS(ContextActor))
		return E->GetEquippedItemData(SlotTag);
	return nullptr;
}

static FGameplayTag FirstValidTag(const TArray<FGameplayTag>& In)
{
	for (const FGameplayTag& T : In)
	{
		if (T.IsValid()) return T;
	}
	return FGameplayTag();
}

void UEquipmentHelperLibrary::GetPreferredEquipSlotsFromItem(UItemDataAsset* ItemData, TArray<FGameplayTag>& Out)
{
	Out.Reset();

	if (!ItemData) return;

	// Read a property named PreferredEquipSlots (TArray<FGameplayTag>) if it exists,
	// so we don't hard-couple this library to a specific subclass.
	static const FName PropName(TEXT("PreferredEquipSlots"));
	if (FProperty* P = ItemData->GetClass()->FindPropertyByName(PropName))
	{
		if (FArrayProperty* Arr = CastField<FArrayProperty>(P))
		{
			if (Arr->Inner && Arr->Inner->IsA<FStructProperty>() &&
				CastFieldChecked<FStructProperty>(Arr->Inner)->Struct == TBaseStructure<FGameplayTag>::Get())
			{
				FScriptArrayHelper Helper(Arr, Arr->ContainerPtrToValuePtr<void>(ItemData));
				const int32 Num = Helper.Num();
				for (int32 i = 0; i < Num; ++i)
				{
					const uint8* Elem = Helper.GetRawPtr(i);
					Out.Add(*reinterpret_cast<const FGameplayTag*>(Elem));
				}
			}
		}
	}
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(AActor* ContextActor,
	UInventoryComponent* SourceInventory, int32 SourceIndex,
	const FGameplayTag& PreferredSlot, bool bAlsoWield)
{
	if (!ContextActor || !SourceInventory)
		return false;

	// Peek the item to consult its preferred slots
	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	UItemDataAsset* Data = nullptr;

	if (!Item.ItemData.IsNull())
		Data = Item.ItemData.LoadSynchronous();

	if (!Data && Item.ItemIDTag.IsValid())
		Data = UInventoryAssetManager::Get().LoadItemDataByTag(Item.ItemIDTag, /*sync*/ true);

	if (!Data && !Item.ItemIDTag.IsValid())
		return false;

	TArray<FGameplayTag> Choices;
	GetPreferredEquipSlotsFromItem(Data, Choices);

	FGameplayTag FinalSlot = FirstValidTag(Choices);
	if (!FinalSlot.IsValid())
		FinalSlot = PreferredSlot; // last resort, caller’s desired slot/root

	if (!FinalSlot.IsValid())
		return false;

	if (UEquipmentComponent* Equip = GetEquipmentPS(ContextActor))
	{
		const bool bEquipped = Equip->TryEquipByInventoryIndex(FinalSlot, SourceInventory, SourceIndex);
		if (!bEquipped) return false;

		if (bAlsoWield)
		{
			if (UWieldComponent* Wield = GetWieldPS(ContextActor))
			{
				// Let wield component auto-holster into hands if it wants
				Wield->TryWieldEquippedInSlot(FinalSlot);
			}
		}
		return true;
	}

	return false;
}
