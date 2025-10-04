// All rights Reserved Midnight Entertainment Studios LLC
#include "EquipmentSystem/EquipmentHelperLibrary.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "EquipmentSystem/EquipmentComponent.h"
#include "EquipmentSystem/WieldComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"

APlayerState* UEquipmentHelperLibrary::ResolvePlayerState(AActor* OwnerLike)
{
	if (!OwnerLike) return nullptr;

	if (APlayerState* AsPS = Cast<APlayerState>(OwnerLike))
		return AsPS;

	if (APawn* AsPawn = Cast<APawn>(OwnerLike))
		return AsPawn->GetPlayerState();

	if (APlayerController* PC = Cast<APlayerController>(OwnerLike))
		return PC->PlayerState;

	if (const APawn* InstPawn = Cast<APawn>(OwnerLike->GetInstigator()))
		return InstPawn->GetPlayerState();

	return nullptr;
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UEquipmentComponent>() : nullptr;
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentPS(AActor* OwnerLike)
{
	return GetEquipmentPS(ResolvePlayerState(OwnerLike));
}

UEquipmentComponent* UEquipmentHelperLibrary::GetEquipmentAny(AActor* OwnerLike)
{
	if (!OwnerLike) return nullptr;
	if (UEquipmentComponent* Direct = OwnerLike->FindComponentByClass<UEquipmentComponent>())
		return Direct;
	return GetEquipmentPS(OwnerLike);
}

UWieldComponent* UEquipmentHelperLibrary::GetWieldPS(AActor* OwnerLike)
{
	if (!OwnerLike) return nullptr;
	if (UWieldComponent* Direct = OwnerLike->FindComponentByClass<UWieldComponent>())
		return Direct;
	if (APlayerState* PS = ResolvePlayerState(OwnerLike))
		return PS->FindComponentByClass<UWieldComponent>();
	return nullptr;
}

UItemDataAsset* UEquipmentHelperLibrary::GetEquippedItemData(AActor* OwnerLike, FGameplayTag SlotTag)
{
	if (UEquipmentComponent* Equip = GetEquipmentAny(OwnerLike))
		return Equip->GetEquippedItemData(SlotTag);
	return nullptr;
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(
	APlayerState* PlayerState,
	UInventoryComponent* SourceInventory,
	int32 SourceIndex,
	FGameplayTag PreferredSlot,
	bool /*bAlsoWield*/)
{
	if (!PlayerState) return false;
	if (UEquipmentComponent* Equip = PlayerState->FindComponentByClass<UEquipmentComponent>())
		return Equip->TryEquipByInventoryIndex(PreferredSlot, SourceInventory, SourceIndex);
	return false;
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(
	APawn* Pawn,
	UInventoryComponent* SourceInventory,
	int32 SourceIndex,
	FGameplayTag PreferredSlot,
	bool bAlsoWield)
{
	return EquipBestFromInventoryIndex(Pawn ? Pawn->GetPlayerState() : nullptr, SourceInventory, SourceIndex, PreferredSlot, bAlsoWield);
}

bool UEquipmentHelperLibrary::EquipBestFromInventoryIndex(
	AActor* OwnerLike,
	UInventoryComponent* SourceInventory,
	int32 SourceIndex,
	FGameplayTag PreferredSlot,
	bool bAlsoWield)
{
	return EquipBestFromInventoryIndex(ResolvePlayerState(OwnerLike), SourceInventory, SourceIndex, PreferredSlot, bAlsoWield);
}
