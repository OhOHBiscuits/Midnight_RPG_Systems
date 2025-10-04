// InventoryComponent.cpp
//
#include "Inventory/InventoryComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ItemDataAsset.h"
//
static FGameplayTag GT_Public()   { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Public"),   false); }
static FGameplayTag GT_ViewOnly() { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.ViewOnly"), false); }
static FGameplayTag GT_Private()  { return FGameplayTag::RequestGameplayTag(TEXT("Inventory.Access.Private"),  false); }
//
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	AdjustSlotCountIfNeeded();
	RecalculateWeightAndVolume();
}
void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
	DOREPLIFETIME(UInventoryComponent, AccessTag);
}
void UInventoryComponent::OnRep_InventoryItems()
{
	const int32 OldNum = ClientPrevItems.Num();
	const int32 NewNum = Items.Num();

	
	const int32 Overlap = FMath::Min(OldNum, NewNum);
	for (int32 i = 0; i < Overlap; ++i)
	{
		if (!ItemsEqual_Client(ClientPrevItems[i], Items[i]))
		{
			OnInventoryUpdated.Broadcast(i);
		}
	}

	
	if (OldNum != NewNum)
	{
		OnInventoryChanged.Broadcast();
	}
	else
	{
		
		OnInventoryChanged.Broadcast();
	}

	ClientPrevItems = Items; 
}
void UInventoryComponent::OnRep_AccessTag(){}
void UInventoryComponent::SetInventoryAccess(const FGameplayTag& NewAccessTag)
{
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority())
		{
			AccessTag = NewAccessTag;
			OnRep_AccessTag();
		}
	}
}
bool UInventoryComponent::HasAccess_Public()   const { return AccessTag == GT_Public(); }
bool UInventoryComponent::HasAccess_ViewOnly() const { return AccessTag == GT_ViewOnly(); }
bool UInventoryComponent::HasAccess_Private()  const { return AccessTag == GT_Private(); }
void UInventoryComponent::AutoInitializeAccessFromArea(AActor*){}
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
void UInventoryComponent::NotifySlotChanged(int32 SlotIndex)
{
	OnInventoryUpdated.Broadcast(SlotIndex);
}
void UInventoryComponent::NotifyInventoryChanged()
{
	const bool PrevFull = bWasFull;
	RecalculateWeightAndVolume();

	const bool NowFull = IsInventoryFull();
	if (NowFull != PrevFull)
	{
		bWasFull = NowFull;
		OnInventoryFull.Broadcast(NowFull);
	}

	OnInventoryChanged.Broadcast();
}
void UInventoryComponent::UpdateItemIndexes()
{
	for (int32 i = 0; i < Items.Num(); ++i) Items[i].Index = i;
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
void UInventoryComponent::SetMaxCarryWeight(float NewMaxWeight) { MaxCarryWeight = FMath::Max(0.f, NewMaxWeight); }
void UInventoryComponent::SetMaxCarryVolume(float NewMaxVolume) { MaxCarryVolume = FMath::Max(0.f, NewMaxVolume); }
void UInventoryComponent::SetMaxSlots(int32 NewMaxSlots)
{
	MaxSlots = FMath::Max(0, NewMaxSlots);
	AdjustSlotCountIfNeeded();
	NotifyInventoryChanged();
}
// Queries//
bool UInventoryComponent::IsInventoryFull() const { return GetNumOccupiedSlots() >= MaxSlots; }
float UInventoryComponent::GetCurrentWeight() const { return CurrentWeight; }
float UInventoryComponent::GetCurrentVolume() const { return CurrentVolume; }
FInventoryItem UInventoryComponent::GetItem(int32 SlotIndex) const { return Items.IsValidIndex(SlotIndex) ? Items[SlotIndex] : FInventoryItem(); }
int32 UInventoryComponent::FindFreeSlot() const { for (int32 i=0;i<Items.Num();++i) if(!Items[i].IsValid()) return i; return INDEX_NONE; }
int32 UInventoryComponent::FindStackableSlot(UItemDataAsset* ItemData) const
{
	if (!ItemData) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i){ const FInventoryItem& S=Items[i]; if(S.IsValid() && S.CanStackWith(ItemData)) return i; }
	return INDEX_NONE;
}
int32 UInventoryComponent::FindSlotWithItemID(FGameplayTag ItemID) const
{
	if (!ItemID.IsValid()) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i){ const FInventoryItem& S=Items[i]; if(!S.IsValid()) continue; if(const UItemDataAsset* D=S.ItemData.Get()){ if (D->ItemIDTag==ItemID) return i; } }
	return INDEX_NONE;
}
FInventoryItem UInventoryComponent::GetItemByID(FGameplayTag ItemID) const
{
	const int32 Idx = FindSlotWithItemID(ItemID);
	return Idx != INDEX_NONE ? Items[Idx] : FInventoryItem();
}
int32 UInventoryComponent::GetNumOccupiedSlots() const { int32 C=0; for(const FInventoryItem& S:Items) if(S.IsValid()) ++C; return C; }
int32 UInventoryComponent::GetNumItemsOfType(FGameplayTag ItemID) const
{
	if(!ItemID.IsValid()) return 0; int32 T=0;
	for(const FInventoryItem& S:Items){ if(!S.IsValid()) continue; if(const UItemDataAsset* D=S.ItemData.Get()){ if(D->ItemIDTag==ItemID) T+=S.Quantity; } }
	return T;
}
int32 UInventoryComponent::GetNumUISlots() const { return Items.Num(); }
void UInventoryComponent::GetUISlotInfo(TArray<int32>& OutIdx, TArray<UItemDataAsset*>& OutData, TArray<int32>& OutQty) const
{
	OutIdx.Reset(); OutData.Reset(); OutQty.Reset();
	for(int32 i=0;i<Items.Num();++i){ const FInventoryItem& S=Items[i]; if(!S.IsValid()) continue; OutIdx.Add(i); OutData.Add(S.ItemData.Get()); OutQty.Add(S.Quantity); }
}
// Filters//
TArray<FInventoryItem> UInventoryComponent::FilterItemsByRarity(FGameplayTag RarityTag) const
{
	TArray<FInventoryItem> Out; if(!RarityTag.IsValid()) return Out;
	for(const FInventoryItem& S:Items) if(S.IsValid()) if(const UItemDataAsset* D=S.ItemData.Get()) if(D->Rarity==RarityTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByCategory(FGameplayTag CategoryTag) const
{
	TArray<FInventoryItem> Out; if(!CategoryTag.IsValid()) return Out;
	for(const FInventoryItem& S:Items) if(S.IsValid()) if(const UItemDataAsset* D=S.ItemData.Get()) if(D->ItemCategory==CategoryTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsBySubCategory(FGameplayTag SubCategoryTag) const
{
	TArray<FInventoryItem> Out; if(!SubCategoryTag.IsValid()) return Out;
	for(const FInventoryItem& S:Items) if(S.IsValid()) if(const UItemDataAsset* D=S.ItemData.Get()) if(D->ItemSubCategory==SubCategoryTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByType(FGameplayTag TypeTag) const
{
	TArray<FInventoryItem> Out; if(!TypeTag.IsValid()) return Out;
	for(const FInventoryItem& S:Items) if(S.IsValid()) if(const UItemDataAsset* D=S.ItemData.Get()) if(D->ItemType==TypeTag) Out.Add(S);
	return Out;
}
TArray<FInventoryItem> UInventoryComponent::FilterItemsByTags(FGameplayTagContainer Tags, bool bMatchAll) const
{
	TArray<FInventoryItem> Out; if(Tags.IsEmpty()) return Out;
	for(const FInventoryItem& S:Items){ if(!S.IsValid()) continue; const UItemDataAsset* D=S.ItemData.Get(); if(!D) continue;
		FGameplayTagContainer Owned; Owned.AddTag(D->ItemIDTag); Owned.AddTag(D->Rarity); Owned.AddTag(D->ItemCategory); Owned.AddTag(D->ItemSubCategory); Owned.AddTag(D->ItemType);
		for(const FGameplayTag& T : D->AdditionalTags) Owned.AddTag(T);
		const bool bOk = bMatchAll ? Owned.HasAll(Tags) : Owned.HasAny(Tags);
		if(bOk) Out.Add(S);
	}
	return Out;
}
bool UInventoryComponent::CanAcceptItem(UItemDataAsset* ItemData) const
{
	if(!ItemData) return false;
	if(AllowedItemIDs.Num()==0) return true;
	for(const FGameplayTag& T:AllowedItemIDs) if(T.IsValid() && T==ItemData->ItemIDTag) return true;
	return false;
}
// Core actions//
bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity, AActor* Requestor)
{
	if(!ItemData || Quantity<=0) return false;
	if (AActor* O=GetOwner()){ if(!O->HasAuthority()) return TryAddItem(ItemData,Quantity); }
	if(!CanModify(Requestor) || !CanAcceptItem(ItemData)) return false;

	AdjustSlotCountIfNeeded();

	if (int32 Stack=FindStackableSlot(ItemData); Stack!=INDEX_NONE)
	{
		FInventoryItem& Slot=Items[Stack]; Slot.Quantity += Quantity;
		NotifySlotChanged(Stack); OnItemAdded.Broadcast(Slot,Quantity); NotifyInventoryChanged(); return true;
	}
	const int32 Free=FindFreeSlot(); if(Free==INDEX_NONE) return false;

	FInventoryItem NewI; NewI.ItemData=ItemData; NewI.Quantity=Quantity; NewI.Index=Free;
	Items[Free]=NewI; NotifySlotChanged(Free); OnItemAdded.Broadcast(NewI,Quantity); NotifyInventoryChanged(); return true;
}
bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity, AActor* Requestor)
{
	if(Quantity<=0 || !Items.IsValidIndex(SlotIndex)) return false;
	if(AActor* O=GetOwner()){ if(!O->HasAuthority()) return TryRemoveItem(SlotIndex,Quantity); }
	if(!CanModify(Requestor)) return false;

	FInventoryItem& S=Items[SlotIndex]; if(!S.IsValid()) return false;
	const int32 RemovedQty = FMath::Min(S.Quantity, Quantity);
	FInventoryItem Removed=S; Removed.Quantity=RemovedQty;

	S.Quantity -= Quantity; if(S.Quantity<=0) S = FInventoryItem();
	NotifySlotChanged(SlotIndex); OnItemRemoved.Broadcast(Removed); NotifyInventoryChanged(); return true;
}
bool UInventoryComponent::RemoveItemByID(FGameplayTag ItemID, int32 Quantity, AActor* Requestor)
{
	if(!ItemID.IsValid()||Quantity<=0) return false;
	const int32 SlotIndex=FindSlotWithItemID(ItemID);
	return RemoveItem(SlotIndex,Quantity,Requestor);
}
bool UInventoryComponent::MoveItem(int32 FromIndex, int32 ToIndex, AActor* Requestor)
{
	if(!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex==ToIndex) return false;
	if(AActor* O=GetOwner()){ if(!O->HasAuthority()) return TryMoveItem(FromIndex,ToIndex); }
	if(!CanModify(Requestor)) return false;

	FInventoryItem& A=Items[FromIndex]; FInventoryItem& B=Items[ToIndex];
	if(!A.IsValid()) return false;

	if(B.IsValid() && A.CanStackWith(B.ItemData.Get())) { B.Quantity += A.Quantity; A=FInventoryItem(); }
	else { Swap(A,B); }

	NotifySlotChanged(FromIndex); NotifySlotChanged(ToIndex); NotifyInventoryChanged(); return true;
}
bool UInventoryComponent::SwapItems(int32 IndexA, int32 IndexB, AActor* Requestor) { return MoveItem(IndexA, IndexB, Requestor); }
bool UInventoryComponent::TransferItemToInventory(int32 FromIndex, UInventoryComponent* TargetInventory, AActor* Requestor)
{
	if(!TargetInventory) return false;
	if(AActor* O=GetOwner()){ if(!O->HasAuthority()) return TryTransferItem(FromIndex,TargetInventory); }
	if(!CanModify(Requestor) || !Items.IsValidIndex(FromIndex)) return false;

	FInventoryItem& S=Items[FromIndex]; if(!S.IsValid()) return false;
	UItemDataAsset* D=S.ItemData.Get(); if(!TargetInventory->CanAcceptItem(D)) return false;

	if(int32 TStack=TargetInventory->FindStackableSlot(D); TStack!=INDEX_NONE)
	{
		FInventoryItem TSlot=TargetInventory->GetItem(TStack); TSlot.Quantity += S.Quantity;
		TargetInventory->Items[TStack]=TSlot; TargetInventory->NotifySlotChanged(TStack);
		TargetInventory->OnItemAdded.Broadcast(TSlot,S.Quantity);
	}
	else
	{
		const int32 Free=TargetInventory->FindFreeSlot(); if(Free==INDEX_NONE) return false;
		FInventoryItem NewI=S; NewI.Index=Free;
		TargetInventory->Items[Free]=NewI; TargetInventory->NotifySlotChanged(Free);
		TargetInventory->OnItemAdded.Broadcast(NewI,NewI.Quantity);
	}

	FInventoryItem Moved=S; S=FInventoryItem();
	NotifySlotChanged(FromIndex); OnItemRemoved.Broadcast(Moved);
	NotifyInventoryChanged(); TargetInventory->NotifyInventoryChanged(); OnItemTransferSuccess.Broadcast(Moved);
	return true;
}
// Split Stack//
bool UInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitQuantity, AActor* Requestor)
{
	if(AActor* O=GetOwner()){ if(!O->HasAuthority()) return TrySplitStack(SlotIndex,SplitQuantity); }
	if(!CanModify(Requestor)) return false;
	if(!Items.IsValidIndex(SlotIndex)) return false;

	FInventoryItem& Source=Items[SlotIndex]; if(!Source.IsValid()) return false;
	if(SplitQuantity<=0 || SplitQuantity>=Source.Quantity) return false;

	int32 TargetIndex=FindFreeSlot(); if(TargetIndex==INDEX_NONE){ TargetIndex=Items.AddDefaulted(1); UpdateItemIndexes(); }
	FInventoryItem NewStack; NewStack.ItemData=Source.ItemData; NewStack.Quantity=SplitQuantity; NewStack.Index=TargetIndex;
	Source.Quantity -= SplitQuantity; Items[TargetIndex]=NewStack;

	NotifySlotChanged(SlotIndex); NotifySlotChanged(TargetIndex); OnItemAdded.Broadcast(NewStack,NewStack.Quantity); NotifyInventoryChanged(); return true;
}
// High-level push/pull
bool UInventoryComponent::PushToInventory(UInventoryComponent* TargetInventory, int32 FromIndex, int32 TargetIndex /*= -1*/)
{
	if (!TargetInventory) return false;
	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority())
		{
			if (TargetIndex >= 0) { return RequestTransferItem(this, FromIndex, TargetInventory, TargetIndex); }
			return TransferItemToInventory(FromIndex, TargetInventory, O);
		}
		return RequestTransferItem(this, FromIndex, TargetInventory, TargetIndex);
	}
	return false;
}
bool UInventoryComponent::PullFromInventory(UInventoryComponent* SourceInventory, int32 SourceIndex, int32 TargetIndex /*= -1*/)
{
	if (!SourceInventory) return false;
	if (AActor* O = GetOwner())
	{
		return RequestTransferItem(SourceInventory, SourceIndex, this, TargetIndex);
	}
	return false;
}
bool UInventoryComponent::RequestTransferItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* TargetInventory, int32 TargetIndex /*= -1*/)
{
	if (!SourceInventory || !TargetInventory) return false;

	if (AActor* O = GetOwner())
	{
		if (O->HasAuthority())
		{
			if(!SourceInventory->Items.IsValidIndex(SourceIndex)) return false;
			FInventoryItem It = SourceInventory->Items[SourceIndex]; if(!It.IsValid()) return false;
			if(!TargetInventory->CanAcceptItem(It.ItemData.Get())) return false;

			if(TargetIndex>=0 && TargetInventory->Items.IsValidIndex(TargetIndex) && !TargetInventory->Items[TargetIndex].IsValid())
			{
				TargetInventory->Items[TargetIndex] = It; TargetInventory->Items[TargetIndex].Index = TargetIndex;
				TargetInventory->OnItemAdded.Broadcast(TargetInventory->Items[TargetIndex], TargetInventory->Items[TargetIndex].Quantity);
			}
			else
			{
				int32 Free = TargetInventory->FindFreeSlot(); if(Free==INDEX_NONE) return false;
				TargetInventory->Items[Free] = It; TargetInventory->Items[Free].Index = Free;
				TargetInventory->OnItemAdded.Broadcast(TargetInventory->Items[Free], TargetInventory->Items[Free].Quantity);
			}

			SourceInventory->OnItemRemoved.Broadcast(It);
			SourceInventory->Items[SourceIndex] = FInventoryItem();
			SourceInventory->NotifySlotChanged(SourceIndex);
			SourceInventory->NotifyInventoryChanged();
			TargetInventory->NotifyInventoryChanged();
			return true;
		}

		Server_TransferItem(SourceInventory, SourceIndex, TargetInventory, TargetIndex, ResolveRequestorController(O));
		return true;
	}
	return false;
}
// Client helpers
bool UInventoryComponent::TryAddItem(UItemDataAsset* ItemData,int32 Quantity)
{
	if(!ItemData||Quantity<=0) return false;
	if (AActor* O=GetOwner()){ if(O->HasAuthority()) return AddItem(ItemData,Quantity,O);
		ServerAddItem(ItemData,Quantity,ResolveRequestorController(O)); return true; }
	return false;
}
bool UInventoryComponent::TryRemoveItem(int32 SlotIndex,int32 Quantity)
{
	if (AActor* O=GetOwner()){ if(O->HasAuthority()) return RemoveItem(SlotIndex,Quantity,O);
		ServerRemoveItem(SlotIndex,Quantity,ResolveRequestorController(O)); return true; }
	return false;
}
bool UInventoryComponent::TryMoveItem(int32 FromIndex,int32 ToIndex)
{
	if (AActor* O=GetOwner()){ if(O->HasAuthority()) return MoveItem(FromIndex,ToIndex,O);
		ServerMoveItem(FromIndex,ToIndex,ResolveRequestorController(O)); return true; }
	return false;
}
bool UInventoryComponent::TryTransferItem(int32 FromIndex,UInventoryComponent* TargetInventory)
{
	if(!TargetInventory) return false;
	if (AActor* O=GetOwner()){ if(O->HasAuthority()) return TransferItemToInventory(FromIndex,TargetInventory,O);
		ServerTransferItem(FromIndex,TargetInventory,ResolveRequestorController(O)); return true; }
	return false;
}
bool UInventoryComponent::TrySplitStack(int32 SlotIndex,int32 SplitQuantity)
{
	if (AActor* O=GetOwner()){ if(O->HasAuthority()) return SplitStack(SlotIndex,SplitQuantity,O);
		ServerSplitStack(SlotIndex,SplitQuantity,ResolveRequestorController(O)); return true; }
	return false;
}
// Sorting//
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
void UInventoryComponent::RequestSortInventoryByName(){ if (AActor* O=GetOwner()) if(!O->HasAuthority()) ServerSortInventoryByName(ResolveRequestorController(O)); else SortInventoryByName(); }
void UInventoryComponent::SortInventoryByRarity()
{
	Items.Sort([](const FInventoryItem& A,const FInventoryItem& B){
		const UItemDataAsset* AD=A.ItemData.Get(); const UItemDataAsset* BD=B.ItemData.Get();
		const FString AR = AD ? AD->Rarity.ToString() : TEXT(""); const FString BR = BD ? BD->Rarity.ToString() : TEXT("");
		return AR < BR;
	});
	UpdateItemIndexes(); NotifyInventoryChanged();
}
void UInventoryComponent::ServerSortInventoryByRarity_Implementation(AController*){ SortInventoryByRarity(); }
bool UInventoryComponent::ServerSortInventoryByRarity_Validate(AController*){ return true; }
void UInventoryComponent::RequestSortInventoryByRarity(){ if (AActor* O=GetOwner()) if(!O->HasAuthority()) ServerSortInventoryByRarity(ResolveRequestorController(O)); else SortInventoryByRarity(); }
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
void UInventoryComponent::RequestSortInventoryByType(){ if (AActor* O=GetOwner()) if(!O->HasAuthority()) ServerSortInventoryByType(ResolveRequestorController(O)); else SortInventoryByType(); }
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
void UInventoryComponent::RequestSortInventoryByCategory(){ if (AActor* O=GetOwner()) if(!O->HasAuthority()) ServerSortInventoryByCategory(ResolveRequestorController(O)); else SortInventoryByCategory(); }
// RPCs//
void UInventoryComponent::ServerAddItem_Implementation(UItemDataAsset* ItemData,int32 Quantity,AController* Requestor){ AddItem(ItemData,Quantity,Requestor); }
bool UInventoryComponent::ServerAddItem_Validate(UItemDataAsset* ItemData,int32 Quantity,AController*){ return ItemData!=nullptr && Quantity>0; }
void UInventoryComponent::ClientAddItemResponse_Implementation(bool /*bSuccess*/){}
void UInventoryComponent::ServerRemoveItem_Implementation(int32 SlotIndex,int32 Quantity,AController* Requestor){ RemoveItem(SlotIndex,Quantity,Requestor); }
bool UInventoryComponent::ServerRemoveItem_Validate(int32 SlotIndex,int32 Quantity,AController*){ return SlotIndex>=0 && Quantity>0; }
void UInventoryComponent::ServerRemoveItemByID_Implementation(FGameplayTag ItemID,int32 Quantity,AController* Requestor){ RemoveItemByID(ItemID,Quantity,Requestor); }
bool UInventoryComponent::ServerRemoveItemByID_Validate(FGameplayTag ItemID,int32 Quantity,AController*){ return ItemID.IsValid() && Quantity>0; }
void UInventoryComponent::ServerMoveItem_Implementation(int32 FromIndex,int32 ToIndex,AController* Requestor){ MoveItem(FromIndex,ToIndex,Requestor); }
bool UInventoryComponent::ServerMoveItem_Validate(int32 FromIndex,int32 ToIndex,AController*){ return FromIndex>=0 && ToIndex>=0 && FromIndex!=ToIndex; }
void UInventoryComponent::ServerTransferItem_Implementation(int32 FromIndex,UInventoryComponent* Target,AController* Requestor){ TransferItemToInventory(FromIndex,Target,Requestor); }
bool UInventoryComponent::ServerTransferItem_Validate(int32 FromIndex,UInventoryComponent* Target,AController*){ return FromIndex>=0 && Target!=nullptr; }
void UInventoryComponent::ServerPullItem_Implementation(int32 FromIndex,UInventoryComponent* Source,AController* Requestor)
{ if (Source) Source->TransferItemToInventory(FromIndex,this,Requestor); }
bool UInventoryComponent::ServerPullItem_Validate(int32 FromIndex,UInventoryComponent* Source,AController*){ return FromIndex>=0 && Source!=nullptr; }
void UInventoryComponent::Server_TransferItem_Implementation(UInventoryComponent* Source,int32 SourceIdx,UInventoryComponent* Target,int32 TargetIdx,AController*)
{
	if(!Source||!Target) return;
	if(!Source->Items.IsValidIndex(SourceIdx)) return;
	FInventoryItem It=Source->Items[SourceIdx]; if(!It.IsValid()) return;
	if(!Target->CanAcceptItem(It.ItemData.Get())) return;

	if(TargetIdx>=0 && Target->Items.IsValidIndex(TargetIdx) && !Target->Items[TargetIdx].IsValid())
	{
		Target->Items[TargetIdx]=It; Target->Items[TargetIdx].Index=TargetIdx;
		Target->OnItemAdded.Broadcast(Target->Items[TargetIdx], Target->Items[TargetIdx].Quantity);
	}
	else
	{
		int32 Free=Target->FindFreeSlot(); if(Free==INDEX_NONE) return;
		Target->Items[Free]=It; Target->Items[Free].Index=Free;
		Target->OnItemAdded.Broadcast(Target->Items[Free], Target->Items[Free].Quantity);
	}

	Source->OnItemRemoved.Broadcast(It);
	Source->Items[SourceIdx]=FInventoryItem();
	Source->NotifySlotChanged(SourceIdx);
	Source->NotifyInventoryChanged();
	Target->NotifyInventoryChanged();
}
bool UInventoryComponent::Server_TransferItem_Validate(UInventoryComponent* Source,int32 SourceIdx,UInventoryComponent* Target,int32 /*TargetIdx*/,AController*){ return Source!=nullptr && Target!=nullptr && SourceIdx>=0; }
bool UInventoryComponent::ServerSplitStack_Validate(int32 SlotIndex, int32 SplitQuantity, AController*){ return SlotIndex >= 0 && SplitQuantity > 0; }
void UInventoryComponent::ServerSplitStack_Implementation(int32 SlotIndex, int32 SplitQuantity, AController* Requestor){ SplitStack(SlotIndex, SplitQuantity, Requestor); }

// Derived state//
void UInventoryComponent::RecalculateWeightAndVolume()
{
	float NewWeight=0.f, NewVolume=0.f;
	for(const FInventoryItem& S:Items)
	{
		if(!S.IsValid()) continue;
		if(const UItemDataAsset* D=S.ItemData.Get())
		{
			NewWeight += D->Weight*S.Quantity;
			NewVolume += D->Volume*S.Quantity;
		}
	}
	constexpr float Eps=0.0001f;
	if(FMath::Abs(NewWeight-CurrentWeight)>Eps){ CurrentWeight=NewWeight; OnWeightChanged.Broadcast(CurrentWeight); }
	if(FMath::Abs(NewVolume-CurrentVolume)>Eps){ CurrentVolume=NewVolume; OnVolumeChanged.Broadcast(CurrentVolume); }
}

bool UInventoryComponent::ItemsEqual_Client(const FInventoryItem& A, const FInventoryItem& B)
{
	const FSoftObjectPath PA = A.ItemData.ToSoftObjectPath();
	const FSoftObjectPath PB = B.ItemData.ToSoftObjectPath();
	return A.Quantity == B.Quantity && PA == PB;
}