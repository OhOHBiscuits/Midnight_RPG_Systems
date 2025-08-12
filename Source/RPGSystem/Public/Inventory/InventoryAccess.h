#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UInventoryComponent;
class AActor;

UENUM(BlueprintType)
enum class EInventoryAccessIntent : uint8
{
	OpenUI,         // view/open container UI
	TransferItem,   // pull/push items
	Manage,         // change settings/privacy/owner
};

UENUM(BlueprintType)
enum class EInventoryAccessResult : uint8
{
	Allowed,
	Denied_NoInventory,
	Denied_NotOwner,
	Denied_Privacy,
	Denied_Locked,
	Denied_AdminOnly,
	Denied_Custom
};

/** Hook you can swap out later (e.g., to integrate with your party/clan system). */
struct FInventoryAccessRules
{
	TFunction<bool(const AActor* Querier, const FString& OwnerId)> IsAdmin     = [](const AActor*, const FString&) { return false; };
	TFunction<bool(const AActor* Querier, const FString& OwnerId)> IsOwner     = [](const AActor* Q, const FString& Owner) {
		// Replace with your player ID fetch
		// Example: compare Owner to PlayerState->GetUniqueId()->ToString()
		return false;
	};
	TFunction<bool(const AActor* Querier, const FString& OwnerId)> InSameGroup = [](const AActor*, const FString&) { return false; };
	TFunction<bool(const AActor* Querier, const FString& OwnerId)> InSameSquad = [](const AActor*, const FString&) { return false; };
	TFunction<bool(const AActor* Querier, const FString& OwnerId)> IsAllowedUser= [](const AActor*, const FString&) { return false; };
};

namespace InventoryAccess
{
	EInventoryAccessResult CanActorAccess(
		const UInventoryComponent* Inv,
		const AActor* Querier,
		EInventoryAccessIntent Intent,
		const FInventoryAccessRules& Rules);
}
