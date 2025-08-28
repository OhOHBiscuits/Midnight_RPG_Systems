#include "Crafting/CraftingQueueComponent.h"
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

#include "Inventory/ItemDataAsset.h"           // <-- needed for UItemDataAsset::StaticClass()
#include "Net/UnrealNetwork.h"

// reflection helpers
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/Script.h"

UCraftingQueueComponent::UCraftingQueueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingQueueComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveStation();

	if (Station)
	{
		Station->OnCraftStarted.AddDynamic(this, &UCraftingQueueComponent::HandleStationStarted);
		Station->OnCraftCompleted.AddDynamic(this, &UCraftingQueueComponent::HandleStationCompleted);
	}
}

void UCraftingQueueComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCraftingQueueComponent, Queue);
	DOREPLIFETIME(UCraftingQueueComponent, bProcessing);
	DOREPLIFETIME(UCraftingQueueComponent, ActiveEntryId);
}

void UCraftingQueueComponent::ResolveStation()
{
	if (!Station)
	{
		if (AActor* Owner = GetOwner())
		{
			Station = Owner->FindComponentByClass<UCraftingStationComponent>();
		}
	}
}

FGuid UCraftingQueueComponent::EnqueueFor(AActor* Submitter, const FCraftingRequest& Request)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return FGuid();

	FCraftQueueEntry E;
	E.EntryId = FGuid::NewGuid();
	E.Submitter = Submitter;
	E.Request = Request;
	if (!E.Request.XPRecipient.IsValid() && Submitter)
	{
		E.Request.XPRecipient = Submitter;
	}
	E.State = ECraftQueueState::Pending;
	E.TimeSubmitted = GetWorld()->GetTimeSeconds();

	Queue.Add(E);
	OnQueueChanged.Broadcast(Queue);

	if (bAutoStart) StartProcessing();
	return E.EntryId;
}

int32 UCraftingQueueComponent::EnqueueBatchFor(AActor* Submitter, const TArray<FCraftingRequest>& Requests)
{
	int32 Count = 0;
	for (const FCraftingRequest& R : Requests)
	{
		if (EnqueueFor(Submitter, R).IsValid()) ++Count;
	}
	return Count;
}

FGuid UCraftingQueueComponent::Enqueue(const FCraftingRequest& Request)
{
	return EnqueueFor(GetOwner(), Request);
}

int32 UCraftingQueueComponent::EnqueueBatch(const TArray<FCraftingRequest>& Requests)
{
	return EnqueueBatchFor(GetOwner(), Requests);
}

int32 UCraftingQueueComponent::EnqueueRecipeFor(AActor* Submitter, UCraftingRecipeDataAsset* Recipe, int32 Quantity)
{
	if (!Recipe || Quantity <= 0) return 0;
	int32 Count = 0;
	for (int32 i=0; i<Quantity; ++i)
	{
		const FCraftingRequest Req = BuildRequestFromRecipe(Submitter, Recipe);
		if (EnqueueFor(Submitter, Req).IsValid()) ++Count;
	}
	return Count;
}

int32 UCraftingQueueComponent::EnqueueRecipeBatchFor(AActor* Submitter, const TArray<UCraftingRecipeDataAsset*>& Recipes)
{
	int32 Count = 0;
	for (UCraftingRecipeDataAsset* R : Recipes)
	{
		if (!R) continue;
		const FCraftingRequest Req = BuildRequestFromRecipe(Submitter, R);
		if (EnqueueFor(Submitter, Req).IsValid()) ++Count;
	}
	return Count;
}

bool UCraftingQueueComponent::RemovePending(const FGuid& EntryId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	const int32 Index = FindIndexById(EntryId);
	if (Index == INDEX_NONE) return false;

	if (Queue[Index].State == ECraftQueueState::Pending)
	{
		Queue.RemoveAt(Index);
		OnQueueChanged.Broadcast(Queue);
		return true;
	}
	return false;
}

void UCraftingQueueComponent::ClearPending()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (int32 i=Queue.Num()-1; i>=0; --i)
	{
		if (Queue[i].State == ECraftQueueState::Pending)
			Queue.RemoveAt(i);
	}
	OnQueueChanged.Broadcast(Queue);
}

void UCraftingQueueComponent::StartProcessing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	ResolveStation();
	if (!Station) return;

	bProcessing = true;
	KickIfPossible();
}

void UCraftingQueueComponent::StopProcessing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	bProcessing = false;
}

int32 UCraftingQueueComponent::FindIndexById(const FGuid& Id) const
{
	for (int32 i=0; i<Queue.Num(); ++i)
	{
		if (Queue[i].EntryId == Id) return i;
	}
	return INDEX_NONE;
}

int32 UCraftingQueueComponent::FindNextPending() const
{
	for (int32 i=0; i<Queue.Num(); ++i)
	{
		if (Queue[i].State == ECraftQueueState::Pending) return i;
	}
	return INDEX_NONE;
}

void UCraftingQueueComponent::KickIfPossible()
{
	if (!bProcessing || !Station) return;
	if (Station->bIsCrafting) return;

	const int32 NextIdx = FindNextPending();
	if (NextIdx == INDEX_NONE) return;

	FCraftQueueEntry& E = Queue[NextIdx];

	const bool bStarted = Station->StartCraft(E.Submitter.Get(), E.Request);
	if (!bStarted)
	{
		E.State = ECraftQueueState::Failed;
		OnQueueChanged.Broadcast(Queue);
		OnEntryCompleted.Broadcast(E, /*bSuccess*/false);
		KickIfPossible();
		return;
	}

	E.State = ECraftQueueState::InProgress;
	E.TimeStarted = GetWorld()->GetTimeSeconds();
	ActiveEntryId = E.EntryId;

	OnQueueChanged.Broadcast(Queue);
}

void UCraftingQueueComponent::HandleStationStarted(const FCraftingRequest& /*Request*/, float /*FinalTime*/, const FSkillCheckResult& /*Check*/)
{
	const int32 Idx = FindIndexById(ActiveEntryId);
	if (Idx != INDEX_NONE)
	{
		OnEntryStarted.Broadcast(Queue[Idx]);
	}
}

void UCraftingQueueComponent::HandleStationCompleted(const FCraftingRequest& /*Request*/, const FSkillCheckResult& /*Check*/, bool bSuccess)
{
	const int32 Idx = FindIndexById(ActiveEntryId);
	if (Idx != INDEX_NONE)
	{
		Queue[Idx].State = bSuccess ? ECraftQueueState::Done : ECraftQueueState::Failed;
		OnQueueChanged.Broadcast(Queue);
		OnEntryCompleted.Broadcast(Queue[Idx], bSuccess);
	}

	ActiveEntryId.Invalidate();
	KickIfPossible();
}

// =================== reflection helpers ===================

static FArrayProperty* FindArrayProp(UClass* Cls, const TCHAR* Name)
{
	return FindFProperty<FArrayProperty>(Cls, FName(Name));
}

template<typename T>
static T* FindObjectField(UStruct* S, void* Container, std::initializer_list<const TCHAR*> Names)
{
	for (const TCHAR* N : Names)
	{
		if (FObjectProperty* P = FindFProperty<FObjectProperty>(S, FName(N)))
		{
			if (P->PropertyClass->IsChildOf(T::StaticClass()))
			{
				return *P->ContainerPtrToValuePtr<T*>(Container);
			}
		}
	}
	return nullptr;
}

static int32 FindIntField(UStruct* S, void* Container, std::initializer_list<const TCHAR*> Names, int32 Default=1)
{
	for (const TCHAR* N : Names)
	{
		if (FIntProperty* P = FindFProperty<FIntProperty>(S, FName(N)))
		{
			return P->GetPropertyValue_InContainer(Container);
		}
	}
	return Default;
}

static FGameplayTag FindTagField(UStruct* S, void* Container, std::initializer_list<const TCHAR*> Names)
{
	for (const TCHAR* N : Names)
	{
		if (FStructProperty* P = FindFProperty<FStructProperty>(S, FName(N)))
		{
			if (P->Struct == TBaseStructure<FGameplayTag>::Get())
			{
				return *P->ContainerPtrToValuePtr<FGameplayTag>(Container);
			}
		}
	}
	return FGameplayTag();
}

static float TryGetFloatProp(UObject* Obj, std::initializer_list<const TCHAR*> Names, float DefaultVal)
{
	UClass* Cls = Obj->GetClass();
	for (const TCHAR* N : Names)
	{
		if (FFloatProperty* P = FindFProperty<FFloatProperty>(Cls, FName(N)))
		{
			return P->GetPropertyValue_InContainer(Obj);
		}
	}
	return DefaultVal;
}

static void CopyInputsFromRecipe(UCraftingRecipeDataAsset* Recipe, TArray<FCraftItemCost>& OutInputs)
{
	if (!Recipe) return;
	if (FArrayProperty* Arr = FindArrayProp(Recipe->GetClass(), TEXT("Inputs")))
	{
		if (FStructProperty* Inner = CastField<FStructProperty>(Arr->Inner))
		{
			FScriptArrayHelper Helper(Arr, Arr->ContainerPtrToValuePtr<void>(Recipe));
			for (int32 i=0; i<Helper.Num(); ++i)
			{
				void* Elem = Helper.GetRawPtr(i);
				FGameplayTag Tag = FindTagField(Inner->Struct, Elem, {TEXT("ItemID"), TEXT("ItemTag"), TEXT("ID")});
				int32 Qty = FindIntField(Inner->Struct, Elem, {TEXT("Quantity"), TEXT("Qty"), TEXT("Count"), TEXT("Amount")}, 1);

				FCraftItemCost Cost;
				Cost.ItemID = Tag;
				Cost.Quantity = FMath::Max(1, Qty);
				OutInputs.Add(Cost);
			}
		}
	}
}

static void CopyOutputsFromRecipe(UCraftingRecipeDataAsset* Recipe, TArray<FCraftItemOutput>& OutOutputs)
{
	if (!Recipe) return;
	if (FArrayProperty* Arr = FindArrayProp(Recipe->GetClass(), TEXT("Outputs")))
	{
		if (FStructProperty* Inner = CastField<FStructProperty>(Arr->Inner))
		{
			FScriptArrayHelper Helper(Arr, Arr->ContainerPtrToValuePtr<void>(Recipe));
			for (int32 i=0; i<Helper.Num(); ++i)
			{
				void* Elem = Helper.GetRawPtr(i);
				UItemDataAsset* Item = FindObjectField<UItemDataAsset>(Inner->Struct, Elem, {TEXT("Item"), TEXT("ItemAsset"), TEXT("ItemData")});
				int32 Qty = FindIntField(Inner->Struct, Elem, {TEXT("Quantity"), TEXT("Qty"), TEXT("Count"), TEXT("Amount")}, 1);

				if (Item)
				{
					FCraftItemOutput Out;
					Out.Item = Item;
					Out.Quantity = FMath::Max(1, Qty);
					OutOutputs.Add(Out);
				}
			}
		}
	}
}

// ==========================================================

FCraftingRequest UCraftingQueueComponent::BuildRequestFromRecipe(AActor* Submitter, UCraftingRecipeDataAsset* Recipe)
{
	FCraftingRequest Req;

	CopyInputsFromRecipe(Recipe, Req.Inputs);
	CopyOutputsFromRecipe(Recipe, Req.Outputs);

	Req.BaseTimeSeconds = TryGetFloatProp(Recipe, {TEXT("BaseTimeSeconds"), TEXT("CraftTime"), TEXT("TimeToCraft")}, 1.0f);

	Req.XPRecipient = Submitter;
	return Req;
}
