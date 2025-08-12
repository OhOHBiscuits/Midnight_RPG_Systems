#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

// ---------- Setup / Rep ----------

UInventoryComponent::UInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Items.SetNum(MaxSlots);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Default owner id if empty
	if (Access.OwnerStableId.IsEmpty())
	{
		Access.OwnerStableId = UInventoryHelpers::GetStablePlayerId(GetOwner());
	}

	// Auto init from area unless designer set something explicit
	AutoInitializeAccessFromArea(GetOwner());
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
	Super::GetLifetimeReplicatedProps(Out);
	DOREPLIFETIME(UInventoryComponent, Items);
	DOREPLIFETIME(UInventoryComponent, Access);
}

// ---------- Access ----------

void UInventoryComponent::AutoInitializeAccessFromArea(AActor* ContextActor)
{
	if (!ContextActor) ContextActor = GetOwner();
	FGameplayTag AreaTag;
	if (UInventoryHelpers::FindOverlappingAreaTag(ContextActor, AreaTag))
	{
		const FGameplayTag BaseTag   = FGameplayTag::RequestGameplayTag(FName("Area.Base"), false);
		const FGameplayTag PlayerTag = FGameplayTag::RequestGameplayTag(FName("Area.Player"), false);

		if (AreaTag.IsValid() && AreaTag.MatchesTagExact(BaseTag))
		{
			Access.AccessMode = EInventoryAccessMode::Public;
			Access.bAllowAlliesAndParty = true; // per spec
		}
		else if (AreaTag.IsValid() && AreaTag.MatchesTagExact(PlayerTag))
		{
			Access.AccessMode = EInventoryAccessMode::Private; // others view-only by default
			Access.bOthersViewOnlyInPrivate = true;
		}
	}
	// else leave as placed defaults
}

void UInventoryComponent::SetOwnerStableId(const FString& NewId)
{
	Access.OwnerStableId = NewId;
}

void UInventoryComponent::SetAccessMode(EInventoryAccessMode NewMode)
{
	Access.AccessMode = NewMode;
}

static FString GetStableIdForActor(AActor* A)
{
	return UInventoryHelpers::GetStablePlayerId(A);
}

AController* UInventoryComponent::ResolveRequestorController(AActor* ExplicitRequestor) const
{
	if (AController* C = Cast<AController>(ExplicitRequestor)) return C;
	if (APawn* P = Cast<APawn>(ExplicitRequestor)) return P->GetController();
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) return OwnerPawn->GetController();
	return nullptr;
}

bool UInventoryComponent::CanView(AActor* Requestor) const
{
	// Owner always can view
	if (Access.OwnerStableId == GetStableIdForActor(Requestor)) return true;

	switch (Access.AccessMode)
	{
		case EInventoryAccessMode::Public:
			if (Access.bAllowAlliesAndParty) return true; // stub: everyone is ally/party
			return UInventoryHelpers::IsSameParty(GetOwner(), Requestor) || UInventoryHelpers::IsAlly(GetOwner(), Requestor);
		case EInventoryAccessMode::ViewOnly:
			return true;
		case EInventoryAccessMode::Private:
			return Access.bOthersViewOnlyInPrivate; // view-only if configured
	}
	return false;
}

bool UInventoryComponent::HeirloomBypassAllowed(AActor* Requestor, const FInventoryItem* OptionalItem) const
{
	if (!Requestor) return false;
	if (Access.OwnerStableId != GetStableIdForActor(Requestor)) return false; // only owner
	if (!Access.HeirloomItemTag.IsValid()) return false;
	if (!OptionalItem) return true; // used for actions that will check items later
	// If item is provided, ensure it's heirloom
	if (OptionalItem->ItemData.IsValid())
	{
		const UItemDataAsset* D = OptionalItem->ItemData.Get();
		return D && (D->ItemType == Access.HeirloomItemTag || D->AllowedActions.Contains(Access.HeirloomItemTag));
	}
	return false;
}

bool UInventoryComponent::CanModify(AActor* Requestor) const
{
	// Owner can always modify
	if (Access.OwnerStableId == GetStableIdForActor(Requestor)) return true;

	switch (Access.AccessMode)
	{
		case EInventoryAccessMode::Public:
			// Allies/party can modify in base public area
			if (Access.bAllowAlliesAndParty) return true; // permissive by design for now
			return UInventoryHelpers::IsSameParty(GetOwner(), Requestor);
		case EInventoryAccessMode::ViewOnly:
			return false;
		case EInventoryAccessMode::Private:
			// Normally no; except heirloom bypass for owner (handled per-action)
			return false;
	}
	return false;
}

// ---------- Actions (permission-aware) ----------

bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity, AActor* Requestor)
{
	if (!ItemData || Quantity <= 0) return false;
	AController* Req = ResolveRequestorController(Requestor);
	if (!CanModify(Req ? Req->GetPawn() : Requestor))
	{
		// Heirloom bypass if in private and owner is doing this (only relevant when item is heirloom)
		FInventoryItem Temp(ItemData, Quantity);
		if (!(Access.AccessMode == EInventoryAccessMode::Private && HeirloomBypassAllowed(Req ? Req->GetPawn() : Requestor, &Temp)))
			return false;
	}

	if (!CanAcceptItem(ItemData))   return false;

	const int32 StackSlot = FindStackableSlot(ItemData);
	const int32 FreeSlot  = (StackSlot == INDEX_NONE) ? FindFreeSlot() : StackSlot;
	if (FreeSlot == INDEX_NONE) { OnInventoryFull.Broadcast(true); return false; }

	if (IsUnlimited())
	{
		if (Items[FreeSlot].IsValid()) Items[FreeSlot].Quantity += Quantity;
		else Items[FreeSlot] = FInventoryItem(ItemData, Quantity, FreeSlot);

		OnItemAdded.Broadcast(Items[FreeSlot], Quantity);
		if (bAutoSort) RequestSortInventoryByName();
		NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
		OnInventoryFull.Broadcast(false);
		return true;
	}

	// (weight/volume/slot checks unchanged)
	float NewVolume=GetCurrentVolume(), NewWeight=GetCurrentWeight();
	if (IsVolumeLimited()) { NewVolume += ItemData->Volume * Quantity; if (NewVolume > MaxCarryVolume) { OnInventoryFull.Broadcast(true); return false; } }
	if (IsWeightLimited()) { NewWeight += ItemData->Weight * Quantity; if (NewWeight > MaxCarryWeight) { OnInventoryFull.Broadcast(true); return false; } }

	int32 AddQty = Quantity;
	if (IsSlotLimited() && Items[FreeSlot].IsValid())
	{
		AddQty = FMath::Min(Quantity, ItemData->MaxStackSize - Items[FreeSlot].Quantity);
		if (AddQty <= 0) { OnInventoryFull.Broadcast(true); return false; }
	}

	if (Items[FreeSlot].IsValid()) Items[FreeSlot].Quantity += AddQty;
	else Items[FreeSlot] = FInventoryItem(ItemData, AddQty, FreeSlot);

	OnItemAdded.Broadcast(Items[FreeSlot], AddQty);
	if (bAutoSort) RequestSortInventoryByName();
	NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
	OnInventoryFull.Broadcast(IsInventoryFull());
	return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity, AActor* Requestor)
{
	if (!Items.IsValidIndex(SlotIndex) || Quantity <= 0) return false;
	FInventoryItem& Item = Items[SlotIndex];
	if (!Item.IsValid() || Item.Quantity < Quantity) return false;

	AController* Req = ResolveRequestorController(Requestor);

	// Private: allow heirloom withdraw by owner
	const bool bIsHeirloom = HeirloomBypassAllowed(Req ? Req->GetPawn() : Requestor, &Item);
	if (!CanModify(Req ? Req->GetPawn() : Requestor) && !bIsHeirloom)
		return false;

	OnItemRemoved.Broadcast(Item);
	Item.Quantity -= Quantity;
	if (Item.Quantity <= 0) Items[SlotIndex] = FInventoryItem();

	if (bAutoSort) RequestSortInventoryByName();
	NotifySlotChanged(SlotIndex); NotifyInventoryChanged(); UpdateItemIndexes();
	return true;
}

bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity, AActor* Requestor)
{
	const int32 SlotIndex = FindSlotWithItemID(ItemID);
	return RemoveItem(SlotIndex, Quantity, Requestor);
}

bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex, AActor* Requestor)
{
	if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex) return false;
	AController* Req = ResolveRequestorController(Requestor);
	if (!CanModify(Req ? Req->GetPawn() : Requestor)) return false;

	FInventoryItem& From = Items[FromIndex];
	FInventoryItem& To   = Items[ToIndex];
	if (!From.IsValid()) return false;

	if (To.IsValid() && From.ItemData == To.ItemData && From.IsStackable())
	{
		const int32 TransferQty = FMath::Min(From.Quantity, To.ItemData.Get()->MaxStackSize - To.Quantity);
		if (TransferQty <= 0) return false;
		To.Quantity += TransferQty;
		From.Quantity -= TransferQty;
		if (From.Quantity <= 0) From = FInventoryItem();
	}
	else
	{
		Swap(From, To);
	}

	NotifySlotChanged(FromIndex); NotifySlotChanged(ToIndex); NotifyInventoryChanged(); UpdateItemIndexes();
	return true;
}

bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory, AActor* Requestor)
{
	if (!TargetInventory || !Items.IsValidIndex(FromIndex)) return false;
	FInventoryItem& Item = Items[FromIndex];
	if (!Item.IsValid()) return false;

	AController* Req = ResolveRequestorController(Requestor);

	const bool bHeirloomBypass = HeirloomBypassAllowed(Req ? Req->GetPawn() : Requestor, &Item);
	if (!CanModify(Req ? Req->GetPawn() : Requestor) && !bHeirloomBypass) return false;

	UItemDataAsset* ItemData = Item.ItemData.Get();
	const int32 Quantity = Item.Quantity;

	// Target does its own permission check with same Requestor
	if (TargetInventory->AddItem(ItemData, Quantity, Req ? Req->GetPawn() : Requestor))
	{
		Items[FromIndex] = FInventoryItem();
		OnItemTransferSuccess.Broadcast(Item);
		NotifySlotChanged(FromIndex); NotifyInventoryChanged(); UpdateItemIndexes();
		TargetInventory->NotifyInventoryChanged();
		return true;
	}
	return false;
}

// ---------- Client convenience (RPC) ----------

bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData, int32 Quantity)
{
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority()) return AddItem(ItemData, Quantity, Owner);
		if (AController* C = Cast<AController>(Owner->GetInstigatorController()))
			ServerAddItem(ItemData, Quantity, C);
		else
			ServerAddItem(ItemData, Quantity, nullptr);
		return true;
	}
	return false;
}

bool UInventoryComponent::TryRemoveItem(int32 SlotIndex, int32 Quantity)
{
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority()) return RemoveItem(SlotIndex, Quantity, Owner);
		if (AController* C = Cast<AController>(Owner->GetInstigatorController()))
			ServerRemoveItem(SlotIndex, Quantity, C);
		else
			ServerRemoveItem(SlotIndex, Quantity, nullptr);
		return false;
	}
	return false;
}

bool UInventoryComponent::TryMoveItem(int32 FromIndex, int32 ToIndex)
{
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority()) return MoveItem(FromIndex, ToIndex, Owner);
		if (AController* C = Cast<AController>(Owner->GetInstigatorController()))
			ServerMoveItem(FromIndex, ToIndex, C);
		else
			ServerMoveItem(FromIndex, ToIndex, nullptr);
		return false;
	}
	return false;
}

bool UInventoryComponent::TryTransferItem(int32 FromIndex, UInventoryComponent* TargetInventory)
{
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority()) return TransferItemToInventory(FromIndex, TargetInventory, Owner);
		if (AController* C = Cast<AController>(Owner->GetInstigatorController()))
			ServerTransferItem(FromIndex, TargetInventory, C);
		else
			ServerTransferItem(FromIndex, TargetInventory, nullptr);
		return false;
	}
	return false;
}

bool UInventoryComponent::RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex)
{
	if (!SourceInventory || !TargetInventory) return false;
	AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
		if (!Item.IsValid()) return false;
		// Permission: we treat Owner as requestor
		if (!TargetInventory->AddItem(Item.ItemData.Get(), Item.Quantity, Owner)) return false;
		SourceInventory->RemoveItem(SourceIndex, Item.Quantity, Owner);
		OnItemTransferSuccess.Broadcast(Item);
		return true;
	}
	if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr))
		Server_TransferItem(SourceInventory, SourceIndex, TargetInventory, TargetIndex, C);
	else
		Server_TransferItem(SourceInventory, SourceIndex, TargetInventory, TargetIndex, nullptr);
	return false;
}

// ---------- Split / Sort (permission guarded where needed) ----------

bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity, AActor* Requestor)
{
	if (!Items.IsValidIndex(SlotIndex) || SplitQuantity <= 0) return false;
	if (!CanModify(Requestor ? Requestor : GetOwner())) return false;

	FInventoryItem& Item = Items[SlotIndex];
	if (!Item.IsValid() || Item.Quantity <= SplitQuantity) return false;

	const int32 FreeSlot = FindFreeSlot();
	if (FreeSlot == INDEX_NONE) return false;

	Item.Quantity -= SplitQuantity;
	Items[FreeSlot] = FInventoryItem(Item.ItemData.Get(), SplitQuantity, FreeSlot);

	if (bAutoSort) RequestSortInventoryByName();
	NotifySlotChanged(SlotIndex); NotifySlotChanged(FreeSlot); NotifyInventoryChanged(); UpdateItemIndexes();
	return true;
}

void UInventoryComponent::ServerSplitStack_Implementation(int32 SlotIndex, int32 SplitQuantity, AController* Requestor)
{
	SplitStack(SlotIndex, SplitQuantity, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerSplitStack_Validate(int32 SlotIndex, int32 SplitQuantity, AController* /*Requestor*/) { return SplitQuantity > 0 && SlotIndex >= 0; }

bool UInventoryComponent::TrySplitStack(int32 SlotIndex, int32 SplitQuantity)
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		return SplitStack(SlotIndex, SplitQuantity, Owner);
	}
	if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr))
		ServerSplitStack(SlotIndex, SplitQuantity, C);
	else
		ServerSplitStack(SlotIndex, SplitQuantity, nullptr);
	return false;
}

// Sort (same behavior; permission: only modifier may sort)
void UInventoryComponent::SortInventoryByName()
{
	Items.Sort([](const FInventoryItem& A, const FInventoryItem& B)
	{
		const UItemDataAsset* DA = A.ItemData.Get();
		const UItemDataAsset* DB = B.ItemData.Get();
		if (!A.IsValid() && !B.IsValid()) return false;
		if (!A.IsValid()) return false; // empty last
		if (!B.IsValid()) return true;
		const FString NameA = DA ? DA->Name.ToString() : FString();
		const FString NameB = DB ? DB->Name.ToString() : FString();
		return NameA < NameB;
	});
	UpdateItemIndexes();
	NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByName_Implementation(AController* Requestor)
{
	if (CanModify(Requestor ? Requestor->GetPawn() : nullptr)) SortInventoryByName();
}
bool UInventoryComponent::ServerSortInventoryByName_Validate(AController* /*Requestor*/) { return true; }
void UInventoryComponent::RequestSortInventoryByName()
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority()) SortInventoryByName();
	else if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr))
		ServerSortInventoryByName(C);
	else
		ServerSortInventoryByName(nullptr);
}

// (Rarity/Type/Category) same pattern
void UInventoryComponent::SortInventoryByRarity()  { /* same as before */ Items.Sort([](const FInventoryItem& A, const FInventoryItem& B){ /* ... */ const UItemDataAsset* DA=A.ItemData.Get(); const UItemDataAsset* DB=B.ItemData.Get(); if(!A.IsValid()&& !B.IsValid())return false; if(!A.IsValid())return false; if(!B.IsValid())return true; return (DA?DA->Rarity.ToString():FString()) < (DB?DB->Rarity.ToString():FString());}); UpdateItemIndexes(); NotifyInventoryChanged(); }
void UInventoryComponent::SortInventoryByType()    { /* same as before */ Items.Sort([](const FInventoryItem& A, const FInventoryItem& B){ const UItemDataAsset* DA=A.ItemData.Get(); const UItemDataAsset* DB=B.ItemData.Get(); if(!A.IsValid()&& !B.IsValid())return false; if(!A.IsValid())return false; if(!B.IsValid())return true; return (DA?DA->ItemType.ToString():FString()) < (DB?DB->ItemType.ToString():FString());}); UpdateItemIndexes(); NotifyInventoryChanged(); }
void UInventoryComponent::SortInventoryByCategory(){ /* same as before */ Items.Sort([](const FInventoryItem& A, const FInventoryItem& B){ const UItemDataAsset* DA=A.ItemData.Get(); const UItemDataAsset* DB=B.ItemData.Get(); if(!A.IsValid()&& !B.IsValid())return false; if(!A.IsValid())return false; if(!B.IsValid())return true; return (DA?DA->ItemCategory.ToString():FString()) < (DB?DB->ItemCategory.ToString():FString());}); UpdateItemIndexes(); NotifyInventoryChanged(); }

void UInventoryComponent::ServerSortInventoryByRarity_Implementation(AController* Requestor){ if (CanModify(Requestor?Requestor->GetPawn():nullptr)) SortInventoryByRarity(); }
bool UInventoryComponent::ServerSortInventoryByRarity_Validate(AController*){ return true; }
void UInventoryComponent::ServerSortInventoryByType_Implementation(AController* Requestor){ if (CanModify(Requestor?Requestor->GetPawn():nullptr)) SortInventoryByType(); }
bool UInventoryComponent::ServerSortInventoryByType_Validate(AController*){ return true; }
void UInventoryComponent::ServerSortInventoryByCategory_Implementation(AController* Requestor){ if (CanModify(Requestor?Requestor->GetPawn():nullptr)) SortInventoryByCategory(); }
bool UInventoryComponent::ServerSortInventoryByCategory_Validate(AController*){ return true; }

void UInventoryComponent::RequestSortInventoryByRarity(){ AActor* Owner = GetOwner(); if (Owner && Owner->HasAuthority()) SortInventoryByRarity(); else if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr)) ServerSortInventoryByRarity(C); else ServerSortInventoryByRarity(nullptr); }
void UInventoryComponent::RequestSortInventoryByType(){   AActor* Owner = GetOwner(); if (Owner && Owner->HasAuthority()) SortInventoryByType();   else if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr)) ServerSortInventoryByType(C);   else ServerSortInventoryByType(nullptr); }
void UInventoryComponent::RequestSortInventoryByCategory(){AActor* Owner = GetOwner(); if (Owner && Owner->HasAuthority()) SortInventoryByCategory(); else if (AController* C = Cast<AController>(Owner ? Owner->GetInstigatorController() : nullptr)) ServerSortInventoryByCategory(C); else ServerSortInventoryByCategory(nullptr); }

// ---------- Queries / Filters / Misc (unchanged bodies) ----------
/* Keep your existing implementations for:
   IsInventoryFull, GetCurrentWeight, GetCurrentVolume, GetItem, FindFreeSlot, FindStackableSlot,
   FindSlotWithItemID, GetItemByID, GetNumOccupiedSlots, GetNumItemsOfType,
   GetNumUISlots, GetUISlotInfo, NotifySlotChanged, NotifyInventoryChanged, UpdateItemIndexes,
   SetMaxCarryWeight, SetMaxCarryVolume, SetMaxSlots, AdjustSlotCountIfNeeded, OnRep_InventoryItems,
   Filters, CanAcceptItem, SwapItems (now permission checked below)
*/

bool UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB, AActor* Requestor)
{
	if (!CanModify(Requestor ? Requestor : GetOwner())) return false;
	if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB) return false;
	Swap(Items[IndexA], Items[IndexB]);
	UpdateItemIndexes();
	NotifySlotChanged(IndexA);
	NotifySlotChanged(IndexB);
	NotifyInventoryChanged();
	return true;
}

// ---------- RPCs (carry requestor) ----------

void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData, int32 Quantity, AController* Requestor)
{
	const bool bSuccess = AddItem(ItemData, Quantity, Requestor ? Requestor->GetPawn() : nullptr);
	ClientAddItemResponse(bSuccess);
}
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* /*ItemData*/, int32 /*Quantity*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::ClientAddItemResponse_Implementation(bool /*bSuccess*/) {}

void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex, int32 Quantity, AController* Requestor)
{
	RemoveItem(SlotIndex, Quantity, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerRemoveItem_Validate(int32 /*SlotIndex*/, int32 /*Quantity*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID, int32 Quantity, AController* Requestor)
{
	RemoveItemByID(ItemID, Quantity, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerRemoveItemByID_Validate(FGameplayTag /*ItemID*/, int32 /*Quantity*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::ClientRemoveItemResponse_Implementation(bool /*bSuccess*/) {}

void UInventoryComponent::ServerMoveItem_Implementation(int32 FromIndex, int32 ToIndex, AController* Requestor)
{
	MoveItem(FromIndex, ToIndex, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerMoveItem_Validate(int32 /*FromIndex*/, int32 /*ToIndex*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex, UInventoryComponent* TargetInventory, AController* Requestor)
{
	TransferItemToInventory(FromIndex, TargetInventory, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerTransferItem_Validate(int32 /*FromIndex*/, UInventoryComponent* /*TargetInventory*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex, UInventoryComponent* SourceInventory, AController* Requestor)
{
	if (!SourceInventory || SourceInventory == this) return;
	const FInventoryItem FromSlot = SourceInventory->GetItem(FromIndex);
	if (!FromSlot.IsValid()) return;

	UItemDataAsset* Asset = FromSlot.ItemData.Get();
	const int32 Quantity = FromSlot.Quantity;

	if (Asset && AddItem(Asset, Quantity, Requestor ? Requestor->GetPawn() : nullptr))
		SourceInventory->RemoveItem(FromIndex, Quantity, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::ServerPullItem_Validate(int32 /*FromIndex*/, UInventoryComponent* /*SourceInventory*/, AController* /*Requestor*/) { return true; }

void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 /*TargetIndex*/, AController* Requestor)
{
	if (!SourceInventory || !TargetInventory) return;
	const FInventoryItem Item = SourceInventory->GetItem(SourceIndex);
	if (!Item.IsValid()) return;

	if (TargetInventory->AddItem(Item.ItemData.Get(), Item.Quantity, Requestor ? Requestor->GetPawn() : nullptr))
		SourceInventory->RemoveItem(SourceIndex, Item.Quantity, Requestor ? Requestor->GetPawn() : nullptr);
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* /*SourceInventory*/, int32 /*SourceIndex*/, UInventoryComponent* /*TargetInventory*/, int32 /*TargetIndex*/, AController* /*Requestor*/) { return true; }
