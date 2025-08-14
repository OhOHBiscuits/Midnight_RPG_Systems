// InventoryComponent.cpp

#include "Inventory/InventoryComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Actor.h"
#include "Inventory/ItemDataAsset.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

static FGameplayTag GT_Public()   { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Public"),   false); }
static FGameplayTag GT_ViewOnly() { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.ViewOnly"), false); }
static FGameplayTag GT_Private()  { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Private"),  false); }

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	AdjustSlotCountIfNeeded();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
	DOREPLIFETIME(UInventoryComponent, AccessTag);
}
void UInventoryComponent::OnRep_InventoryItems(){ NotifyInventoryChanged(); }
void UInventoryComponent::OnRep_AccessTag(){}
void UInventoryComponent::SetInventoryAccess(const FGameplayTag& NewAccessTag)
{
	if (AActor* O = GetOwner()) if (O->HasAuthority()) { AccessTag = NewAccessTag; OnRep_AccessTag(); }
}
bool UInventoryComponent::HasAccess_Public()   const { return AccessTag == GT_Public(); }
bool UInventoryComponent::HasAccess_ViewOnly() const { return AccessTag == GT_ViewOnly(); }
bool UInventoryComponent::HasAccess_Private()  const { return AccessTag == GT_Private(); }
void UInventoryComponent::AutoInitializeAccessFromArea(AActor* /*ContextActor*/){}
AController* UInventoryComponent::ResolveRequestorController(AActor* ExplicitRequestor) const
{
	if (ExplicitRequestor)
	{
		if (AController* C = Cast<AController>(ExplicitRequestor)) return C;
		if (APawn* P = Cast<APawn>(ExplicitRequestor)) return P->GetController();
		if (ExplicitRequestor->GetInstigatorController()) return ExplicitRequestor->GetInstigatorController();
	}
	if (AActor* O = GetOwner())
	{
		if (APawn* P = Cast<APawn>(O)) return P->GetController();
		if (O->GetInstigatorController()) return O->GetInstigatorController();
	}
	return nullptr;
}
bool UInventoryComponent::CanView(AActor* Requestor) const
{
	if (HasAccess_Public() || HasAccess_ViewOnly()) return true;
	if (HasAccess_Private())
	{
		const AController* Req = ResolveRequestorController(Requestor);
		const AController* Own = ResolveRequestorController(const_cast<UInventoryComponent*>(this)->GetOwner());
		return Req && Own && (Req == Own);
	}
	return true;
}
bool UInventoryComponent::CanModify(AActor* Requestor) const
{
	if (HasAccess_Public()) return true;
	if (HasAccess_ViewOnly()) return false;
	if (HasAccess_Private())
	{
		const AController* Req = ResolveRequestorController(Requestor);
		const AController* Own = ResolveRequestorController(const_cast<UInventoryComponent*>(this)->GetOwner());
		return Req && Own && (Req == Own);
	}
	return true;
}
void UInventoryComponent::NotifySlotChanged(int32 SlotIndex){ OnInventoryUpdated.Broadcast(SlotIndex); }
void UInventoryComponent::NotifyInventoryChanged(){ OnInventoryChanged.Broadcast(); }
void UInventoryComponent::UpdateItemIndexes()
{
	for (int32 i=0;i<Items.Num();++i){ Items[i].Index = i; }
}
void UInventoryComponent::AdjustSlotCountIfNeeded()
{
	MaxSlots = FMath::Max(0, MaxSlots);
	if (Items.Num() != MaxSlots)
	{
		Items.SetNum(MaxSlots);
		UpdateItemIndexes();
	}
}
// Settings
void UInventoryComponent::SetMaxCarryWeight(float NewMaxWeight){ MaxCarryWeight = FMath::Max(0.f, NewMaxWeight); }
void UInventoryComponent::SetMaxCarryVolume(float NewMaxVolume){ MaxCarryVolume = FMath::Max(0.f, NewMaxVolume); }
void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots){ MaxSlots = FMath::Max(0, NewMaxSlots); AdjustSlotCountIfNeeded(); NotifyInventoryChanged(); }
// Queries
bool UInventoryComponent::IsInventoryFull() const{ return GetNumOccupiedSlots() >= MaxSlots; }
float UInventoryComponent::GetCurrentWeight() const{ return 0.f; }
float UInventoryComponent::GetCurrentVolume() const{ return 0.f; }
FInventoryItem UInventoryComponent::GetItem(int32 SlotIndex) const
{
	return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex] : FInventoryItem();
}
int32 UInventoryComponent::FindFreeSlot() const
{
	for (int32 i=0;i<Items.Num();++i) if (!Items[i].IsValid()) return i;
	return INDEX_NONE;
}
int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
	if (!ItemData) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i)
	{
		const FInventoryItem& S = Items[i];
		if (S.IsValid() && S.CanStackWith(ItemData)) return i;
	}
	return INDEX_NONE;
}
int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
	if (!ItemID.IsValid()) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i)
	{
		const FInventoryItem& S = Items[i];
		if (!S.IsValid()) continue;
		if (const UItemDataAsset* D = S.ItemData.Get()) if (D->ItemIDTag == ItemID) return i;
	}
	return INDEX_NONE;
}
FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
	const int32 Idx = FindSlotWithItemID(ItemID);
	return Idx != INDEX_NONE ? Items[Idx] : FInventoryItem();
}
int32 UInventoryComponent::GetNumOccupiedSlots() const
{
	int32 C=0; for (const FInventoryItem& S: Items) if (S.IsValid()) ++C; return C;
}
int32 UInventoryComponent::GetNumItemsOfType(FGameplayTag ItemID) const
{
	if (!ItemID.IsValid()) return 0;
	int32 T=0; for (const FInventoryItem& S: Items) if (S.IsValid()) if (const UItemDataAsset* D=S.ItemData.Get()) if (D->ItemIDTag==ItemID) T+=S.Quantity; return T;
}
int32 UInventoryComponent::GetNumUISlots() const{ return Items.Num(); }
void UInventoryComponent::GetUISlotInfo(TArray<int32>& OutIdx, TArray<UItemDataAsset*>& OutData, TArray<int32>& OutQty) const
{
	OutIdx.Reset(); OutData.Reset(); OutQty.Reset();
	for (int32 i=0;i<Items.Num();++i)
	{
		const FInventoryItem& S = Items[i];
		if (!S.IsValid()) continue;
		OutIdx.Add(i); OutData.Add(S.ItemData.Get()); OutQty.Add(S.Quantity);
	}
}
// Filters
TArray<FInventoryItem> UInventoryComponent::FilterItemsByRarity(FGameplayTag RarityTag) const
{
	TArray<FInventoryItem> Out; if (!RarityTag.IsValid()) return Out;
	for (const FInventoryItem& S: Items) if (S.IsValid()) if (const UItemDataAsset* D=S.ItemData.Get()) if (D->Rarity==RarityTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByCategory(FGameplayTag CategoryTag) const
{
	TArray<FInventoryItem> Out; if (!CategoryTag.IsValid()) return Out;
	for (const FInventoryItem& S: Items) if (S.IsValid()) if (const UItemDataAsset* D=S.ItemData.Get()) if (D->ItemCategory==CategoryTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const
{
	TArray<FInventoryItem> Out; if (!SubCategoryTag.IsValid()) return Out;
	for (const FInventoryItem& S: Items) if (S.IsValid()) if (const UItemDataAsset* D=S.ItemData.Get()) if (D->ItemSubCategory==SubCategoryTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByType(FGameplayTag TypeTag) const
{
	TArray<FInventoryItem> Out; if (!TypeTag.IsValid()) return Out;
	for (const FInventoryItem& S: Items) if (S.IsValid()) if (const UItemDataAsset* D=S.ItemData.Get()) if (D->ItemType==TypeTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll) const
{
	TArray<FInventoryItem> Out; if (Tags.IsEmpty()) return Out;
	for (const FInventoryItem& S: Items)
	{
		if (!S.IsValid()) continue;
		const UItemDataAsset* D = S.ItemData.Get(); if (!D) continue;
		FGameplayTagContainer Owned; Owned.AddTag(D->ItemIDTag); Owned.AddTag(D->Rarity); Owned.AddTag(D->ItemCategory); Owned.AddTag(D->ItemSubCategory); Owned.AddTag(D->ItemType);
		for (const FGameplayTag& T : D->AdditionalTags) Owned.AddTag(T);
		const bool bOk = bMatchAll ? Owned.HasAll(Tags) : Owned.HasAny(Tags);
		if (bOk) Out.Add(S);
	}
	return Out;
}
// Acceptance
bool UInventoryComponent::CanAcceptItem(UItemDataAsset* ItemData) const
{
	if (!ItemData) return false;
	if (AllowedItemIDs.Num()==0) return true;
	for (const FGameplayTag& T: AllowedItemIDs) if (T.IsValid() && T==ItemData->ItemIDTag) return true;
	return false;
}
// Core actions
bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity, AActor* Requestor)
{
	if (!ItemData || Quantity <= 0)
		return false;

	if (AActor* O = GetOwner())
	{
		if (!O->HasAuthority())
			return TryAddItem(ItemData, Quantity);
	}

	if (!CanModify(Requestor) || !CanAcceptItem(ItemData))
		return false;

	AdjustSlotCountIfNeeded();

	int32 Stack = FindStackableSlot(ItemData);
	if (Stack != INDEX_NONE)
	{
		FInventoryItem& SlotItem = Items[Stack];
		SlotItem.Quantity += Quantity;
		NotifySlotChanged(Stack);
		NotifyInventoryChanged();
		return true;
	}

	int32 Free = FindFreeSlot();
	if (Free == INDEX_NONE)
		return false;

	FInventoryItem NewI;
	NewI.ItemData = ItemData;
	NewI.Quantity = Quantity;
	NewI.Index = Free;

	Items[Free] = NewI;
	NotifySlotChanged(Free);
	NotifyInventoryChanged();
	return true;
}
bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity, AActor* Requestor)
{
	if (Quantity<=0 || !Items.IsValidIndex(SlotIndex)) return false;
	if (AActor* O=GetOwner()) if (!O->HasAuthority()) return TryRemoveItem(SlotIndex,Quantity);
	if (!CanModify(Requestor)) return false;

	FInventoryItem& S=Items[SlotIndex]; if (!S.IsValid()) return false;
	S.Quantity-=Quantity; if (S.Quantity<=0) S=FInventoryItem();
	NotifySlotChanged(SlotIndex); NotifyInventoryChanged(); return true;
}
bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity, AActor* Requestor)
{
	if (!ItemID.IsValid()||Quantity<=0) return false;
	const int32 SlotIndex=FindSlotWithItemID(ItemID);
	return RemoveItem(SlotIndex,Quantity,Requestor);
}
bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex, AActor* Requestor)
{
	if (!Items.IsValidIndex(FromIndex)||!Items.IsValidIndex(ToIndex)||FromIndex==ToIndex) return false;
	if (AActor* O=GetOwner()) if (!O->HasAuthority()) return TryMoveItem(FromIndex,ToIndex);
	if (!CanModify(Requestor)) return false;

	FInventoryItem& A=Items[FromIndex]; FInventoryItem& B=Items[ToIndex];
	if (!A.IsValid()) return false;

	if (B.IsValid() && A.CanStackWith(B.ItemData.Get())) { B.Quantity+=A.Quantity; A=FInventoryItem(); }
	else { Swap(A,B); }

	NotifySlotChanged(FromIndex); NotifySlotChanged(ToIndex); NotifyInventoryChanged(); return true;
}
bool UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB, AActor* Requestor){ return MoveItem(IndexA,IndexB,Requestor); }
bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory, AActor* Requestor)
{
	if (!TargetInventory) return false;
	if (AActor* O=GetOwner()) if (!O->HasAuthority()) return TryTransferItem(FromIndex,TargetInventory);
	if (!CanModify(Requestor) || !Items.IsValidIndex(FromIndex)) return false;

	FInventoryItem& S=Items[FromIndex]; if (!S.IsValid()) return false;
	UItemDataAsset* D=S.ItemData.Get(); if (!TargetInventory->CanAcceptItem(D)) return false;

	if (int32 TStack=TargetInventory->FindStackableSlot(D); TStack!=INDEX_NONE)
	{
		auto TS=TargetInventory->GetItem(TStack); TS.Quantity+=S.Quantity; TargetInventory->Items[TStack]=TS; TargetInventory->NotifySlotChanged(TStack);
	}
	else
	{
		int32 Free=TargetInventory->FindFreeSlot(); if (Free==INDEX_NONE) return false;
		FInventoryItem NewI=S; NewI.Index=Free; TargetInventory->Items[Free]=NewI; TargetInventory->NotifySlotChanged(Free);
	}

	FInventoryItem Moved=S; S=FInventoryItem();
	NotifySlotChanged(FromIndex); NotifyInventoryChanged(); TargetInventory->NotifyInventoryChanged(); OnItemTransferSuccess.Broadcast(Moved);
	return true;
}

// Split & sort
bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity, AActor* Requestor)
{
	if (!Items.IsValidIndex(SlotIndex)||SplitQuantity<=0) return false;
	if (AActor* O=GetOwner()) if (!O->HasAuthority()) return TrySplitStack(SlotIndex,SplitQuantity);
	if (!CanModify(Requestor)) return false;

	FInventoryItem& S=Items[SlotIndex]; if (!S.IsValid()||S.Quantity<=SplitQuantity) return false;
	const int32 Free=FindFreeSlot(); if (Free==INDEX_NONE) return false;

	S.Quantity-=SplitQuantity;
	FInventoryItem NewI; NewI.ItemData=S.ItemData; NewI.Quantity=SplitQuantity; NewI.Index=Free;
	Items[Free]=NewI; NotifySlotChanged(SlotIndex); NotifySlotChanged(Free); NotifyInventoryChanged(); return true;
}
void UInventoryComponent::SortInventoryByName()
{
	Items.Sort([](const FInventoryItem& A,const FInventoryItem& B){
		const UItemDataAsset* AD=A.ItemData.Get(); const UItemDataAsset* BD=B.ItemData.Get();
		return (AD?AD->GetName():TEXT("")) < (BD?BD->GetName():TEXT(""));
	});
	UpdateItemIndexes(); NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByName_Implementation(AController*){ SortInventoryByName(); }
bool UInventoryComponent::ServerSortInventoryByName_Validate(AController*){ return true; }
void UInventoryComponent::RequestSortInventoryByName(){ if (AActor* O=GetOwner()) if (!O->HasAuthority()) ServerSortInventoryByName(ResolveRequestorController(O)); else SortInventoryByName(); }
void UInventoryComponent::SortInventoryByRarity()
{
	Items.Sort([](const FInventoryItem& A,const FInventoryItem& B){
		const UItemDataAsset* AD=A.ItemData.Get(); const UItemDataAsset* BD=B.ItemData.Get();
		const FString AR = AD ? AD->Rarity.ToString() : TEXT("");
const FString BR = BD ? BD->Rarity.ToString() : TEXT("");
return AR < BR;
	});
	UpdateItemIndexes(); NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByRarity_Implementation(AController*){ SortInventoryByRarity(); }
bool UInventoryComponent::ServerSortInventoryByRarity_Validate(AController*){ return true; }
void UInventoryComponent::RequestSortInventoryByRarity(){ if (AActor* O=GetOwner()) if (!O->HasAuthority()) ServerSortInventoryByRarity(ResolveRequestorController(O)); else SortInventoryByRarity(); }
//Sorts
void UInventoryComponent::SortInventoryByType()
{
	Items.Sort([](const FInventoryItem& A,const FInventoryItem& B){
		const UItemDataAsset* AD=A.ItemData.Get(); const UItemDataAsset* BD=B.ItemData.Get();
		return (AD?AD->ItemType.ToString():TEXT("")) < (BD?BD->ItemType.ToString():TEXT(""));
	});
	UpdateItemIndexes(); NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByType_Implementation(AController*){ SortInventoryByType(); }
bool UInventoryComponent::ServerSortInventoryByType_Validate(AController*){ return true; }
void UInventoryComponent::RequestSortInventoryByType(){ if (AActor* O=GetOwner()) if (!O->HasAuthority()) ServerSortInventoryByType(ResolveRequestorController(O)); else SortInventoryByType(); }
void UInventoryComponent::SortInventoryByCategory()
{
	Items.Sort([](const FInventoryItem& A,const FInventoryItem& B){
		const UItemDataAsset* AD=A.ItemData.Get(); const UItemDataAsset* BD=B.ItemData.Get();
		return (AD?AD->ItemCategory.ToString():TEXT("")) < (BD?BD->ItemCategory.ToString():TEXT(""));
	});
	UpdateItemIndexes(); NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByCategory_Implementation(AController*){ SortInventoryByCategory(); }
bool UInventoryComponent::ServerSortInventoryByCategory_Validate(AController*){ return true; }
void UInventoryComponent::RequestSortInventoryByCategory(){ if (AActor* O=GetOwner()) if (!O->HasAuthority()) ServerSortInventoryByCategory(ResolveRequestorController(O)); else SortInventoryByCategory(); }
// Client wrappers
bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData,int32 Quantity)
{
	if (!ItemData||Quantity<=0) return false;
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority()) return AddItem(ItemData,Quantity,O);
		ServerAddItem(ItemData,Quantity,ResolveRequestorController(O)); return true;
	}
	return false;
}
bool UInventoryComponent::TryRemoveItem(int32 SlotIndex,int32 Quantity)
{
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority()) return RemoveItem(SlotIndex,Quantity,O);
		ServerRemoveItem(SlotIndex,Quantity,ResolveRequestorController(O)); return true;
	}
	return false;
}
bool UInventoryComponent::TryMoveItem(int32 FromIndex,int32 ToIndex)
{
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority()) return MoveItem(FromIndex,ToIndex,O);
		ServerMoveItem(FromIndex,ToIndex,ResolveRequestorController(O)); return true;
	}
	return false;
}
bool UInventoryComponent::TryTransferItem(int32 FromIndex,UInventoryComponent* TargetInventory)
{
	if (!TargetInventory) return false;
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority()) return TransferItemToInventory(FromIndex,TargetInventory,O);
		ServerTransferItem(FromIndex,TargetInventory,ResolveRequestorController(O)); return true;
	}
	return false;
}
bool UInventoryComponent::TrySplitStack(int32 SlotIndex,int32 SplitQuantity)
{
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority()) return SplitStack(SlotIndex,SplitQuantity,O);
		ServerSplitStack(SlotIndex,SplitQuantity,ResolveRequestorController(O)); return true;
	}
	return false;
}
bool UInventoryComponent::RequestTransferItem(UInventoryComponent* Source,int32 SourceIdx,UInventoryComponent* Target,int32 TargetIdx)
{
	if (!Source||!Target) return false;
	if (AActor* O=GetOwner())
	{
		if (O->HasAuthority())
		{
			if (!Source->Items.IsValidIndex(SourceIdx)) return false;
			FInventoryItem It=Source->Items[SourceIdx]; if (!It.IsValid()) return false;
			if (!Target->CanAcceptItem(It.ItemData.Get())) return false;

			if (TargetIdx>=0 && Target->Items.IsValidIndex(TargetIdx) && !Target->Items[TargetIdx].IsValid())
			{ Target->Items[TargetIdx]=It; Target->Items[TargetIdx].Index=TargetIdx; }
			else
			{ int32 Free=Target->FindFreeSlot(); if (Free==INDEX_NONE) return false; Target->Items[Free]=It; Target->Items[Free].Index=Free; }

			Source->Items[SourceIdx]=FInventoryItem(); Source->NotifySlotChanged(SourceIdx);
			Source->NotifyInventoryChanged(); Target->NotifyInventoryChanged(); return true;
		}
		else
		{
			Server_TransferItem(Source,SourceIdx,Target,TargetIdx,ResolveRequestorController(O)); return true;
		}
	}
	return false;
}
// RPCs
void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData,int32 Quantity,AController* Requestor){ AddItem(ItemData,Quantity,Requestor); }
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* ItemData,int32 Quantity,AController*){ return ItemData!=nullptr && Quantity>0; }
void UInventoryComponent::ClientAddItemResponse_Implementation(bool){}
void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex,int32 Quantity,AController* Requestor){ RemoveItem(SlotIndex,Quantity,Requestor); }
bool UInventoryComponent::ServerRemoveItem_Validate(int32 SlotIndex,int32 Quantity,AController*){ return SlotIndex>=0 && Quantity>0; }
void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID,int32 Quantity,AController* Requestor){ RemoveItemByID(ItemID,Quantity,Requestor); }
bool UInventoryComponent::ServerRemoveItemByID_Validate(FGameplayTag ItemID,int32 Quantity,AController*){ return ItemID.IsValid() && Quantity>0; }
void UInventoryComponent::ServerMoveItem_Implementation(int32 FromIndex,int32 ToIndex,AController* Requestor){ MoveItem(FromIndex,ToIndex,Requestor); }
bool UInventoryComponent::ServerMoveItem_Validate(int32 FromIndex,int32 ToIndex,AController*){ return FromIndex>=0 && ToIndex>=0 && FromIndex!=ToIndex; }
void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex,UInventoryComponent* Target,AController* Requestor){ TransferItemToInventory(FromIndex,Target,Requestor); }
bool UInventoryComponent::ServerTransferItem_Validate(int32 FromIndex,UInventoryComponent* Target,AController*){ return FromIndex>=0 && Target!=nullptr; }
void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex,UInventoryComponent* Source,AController* Requestor){ if (Source) Source->TransferItemToInventory(FromIndex,this,Requestor); }
bool UInventoryComponent::ServerPullItem_Validate(int32 FromIndex,UInventoryComponent* Source,AController*){ return FromIndex>=0 && Source!=nullptr; }
void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* Source,int32 SourceIdx,UInventoryComponent* Target,int32 TargetIdx,AController*)
{
	if (!Source||!Target) return; if (!Source->Items.IsValidIndex(SourceIdx)) return;
	FInventoryItem It=Source->Items[SourceIdx]; if (!It.IsValid()) return; if (!Target->CanAcceptItem(It.ItemData.Get())) return;

	if (TargetIdx>=0 && Target->Items.IsValidIndex(TargetIdx) && !Target->Items[TargetIdx].IsValid())
	{ Target->Items[TargetIdx]=It; Target->Items[TargetIdx].Index=TargetIdx; }
	else
	{ int32 Free=Target->FindFreeSlot(); if (Free==INDEX_NONE) return; Target->Items[Free]=It; Target->Items[Free].Index=Free; }

	Source->Items[SourceIdx]=FInventoryItem(); Source->NotifySlotChanged(SourceIdx);
	Source->NotifyInventoryChanged(); Target->NotifyInventoryChanged();
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* Source,int32 SourceIdx,UInventoryComponent* Target,int32 /*TargetIdx*/,AController*){ return Source!=nullptr && Target!=nullptr && SourceIdx>=0; }

bool UInventoryComponent::ServerSplitStack_Validate(int FromIndex, int SplitAmount, AController* By)
{
	// You can harden this as needed
	return true;
}

void UInventoryComponent::ServerSplitStack_Implementation(int FromIndex, int SplitAmount, AController* By)
{
	const AActor* Owner = GetOwner();
	if (!ensureAlways(Owner && Owner->GetLocalRole() == ROLE_Authority)) return;
	if (!Items.IsValidIndex(FromIndex)) return;

	FInventoryItem& Source = Items[FromIndex];
	if (!Source.IsValid()) return;
	if (SplitAmount <= 0 || SplitAmount >= Source.Quantity) return;

	// Find or make an empty slot
	int32 TargetIndex = INDEX_NONE;
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (!Items[i].IsValid())
		{
			TargetIndex = i;
			break;
		}
	}
	if (TargetIndex == INDEX_NONE)
	{
		TargetIndex = Items.AddDefaulted(1);
	}

	// Create the new stack
	FInventoryItem NewStack;
	NewStack.ItemData = Source.ItemData;
	NewStack.Quantity = SplitAmount;
	NewStack.Index = TargetIndex;

	Source.Quantity -= SplitAmount;
	Items[TargetIndex] = NewStack;
	
}
