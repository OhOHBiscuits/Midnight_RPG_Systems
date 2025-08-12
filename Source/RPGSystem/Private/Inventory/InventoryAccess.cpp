#include "Inventory/InventoryAccess.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

static FGameplayTag TAG_Public     = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.Public"));
static FGameplayTag TAG_Private    = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.Private"));
static FGameplayTag TAG_Group      = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.Group"));
static FGameplayTag TAG_Squad      = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.Squad"));
static FGameplayTag TAG_AdminOnly  = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.AdminOnly"));
static FGameplayTag TAG_Locked     = FGameplayTag::RequestGameplayTag(TEXT("Inventory.Privacy.Locked"));

EInventoryAccessResult InventoryAccess::CanActorAccess(
    const UInventoryComponent* Inv,
    const AActor* Querier,
    EInventoryAccessIntent Intent,
    const FInventoryAccessRules& Rules)
{
    if (!Inv)                       return EInventoryAccessResult::Denied_NoInventory;
    if (!Querier)                   return EInventoryAccessResult::Denied_Custom;
    if (Rules.IsAdmin(Querier, Inv->GetInventoryOwnerId())) return EInventoryAccessResult::Allowed;

    const FGameplayTag Privacy = Inv->GetPrivacyTag();

    if (Privacy == TAG_AdminOnly)   return EInventoryAccessResult::Denied_AdminOnly;
    if (Privacy == TAG_Locked)      return EInventoryAccessResult::Denied_Locked;

    if (Privacy == TAG_Public)      return EInventoryAccessResult::Allowed;
    if (Privacy == TAG_Private)
    {
        if (Rules.IsOwner(Querier, Inv->GetInventoryOwnerId()))  return EInventoryAccessResult::Allowed;
        if (Rules.IsAllowedUser(Querier, Inv->GetInventoryOwnerId())) return EInventoryAccessResult::Allowed;
        return EInventoryAccessResult::Denied_Privacy;
    }
    if (Privacy == TAG_Group)
    {
        if (Rules.IsOwner(Querier, Inv->GetInventoryOwnerId()) || Rules.InSameGroup(Querier, Inv->GetInventoryOwnerId()))
            return EInventoryAccessResult::Allowed;
        return EInventoryAccessResult::Denied_Privacy;
    }
    if (Privacy == TAG_Squad)
    {
        if (Rules.IsOwner(Querier, Inv->GetInventoryOwnerId()) || Rules.InSameSquad(Querier, Inv->GetInventoryOwnerId()))
            return EInventoryAccessResult::Allowed;
        return EInventoryAccessResult::Denied_Privacy;
    }
    // default safe:
    return EInventoryAccessResult::Denied_Privacy;
}
