#include "Crafting/CraftingQueueComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"   // FGameplayEventData
#include "Net/UnrealNetwork.h"

static FGameplayTag TAG_Event_Craft_Start     = FGameplayTag::RequestGameplayTag(FName("Event.Craft.Start"));
static FGameplayTag TAG_Event_Craft_Started   = FGameplayTag::RequestGameplayTag(FName("Event.Craft.Started"));
static FGameplayTag TAG_Event_Craft_Completed = FGameplayTag::RequestGameplayTag(FName("Event.Craft.Completed"));
static FGameplayTag TAG_State_Queued          = FGameplayTag::RequestGameplayTag(FName("Craft.Job.Queued"));
static FGameplayTag TAG_State_InProgress      = FGameplayTag::RequestGameplayTag(FName("Craft.Job.InProgress"));
static FGameplayTag TAG_State_Completed       = FGameplayTag::RequestGameplayTag(FName("Craft.Job.Completed"));
static FGameplayTag TAG_State_Failed          = FGameplayTag::RequestGameplayTag(FName("Craft.Job.Failed"));
static FGameplayTag TAG_State_Cancelled       = FGameplayTag::RequestGameplayTag(FName("Craft.Job.Cancelled"));

UCraftingQueueComponent::UCraftingQueueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingQueueComponent::BeginPlay()
{
	Super::BeginPlay();
	BindASCEvents();
}

void UCraftingQueueComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCraftingQueueComponent, Queue);
}

void UCraftingQueueComponent::OnRep_Queue()
{
	OnQueueChanged.Broadcast();
}

bool UCraftingQueueComponent::EnqueueRecipe(UCraftingRecipeDataAsset* Recipe, int32 Quantity, AActor* Workstation)
{
	if (GetOwnerRole() != ROLE_Authority || !Recipe || Quantity < 1) return false;

	FCraftQueueEntry Entry;
	Entry.RecipeIDTag = Recipe->RecipeIDTag;
	Entry.Quantity    = Quantity;
	Entry.JobId       = FGuid::NewGuid();
	Entry.StateTag    = TAG_State_Queued;
	Entry.ETASeconds  = Recipe->BaseTimeSeconds * Quantity;

	Queue.Add(Entry);
	FServerJob& SJ = ServerJobs.Add(Entry.JobId);
	SJ.Recipe      = Recipe;
	SJ.Workstation = Workstation;

	OnQueueChanged.Broadcast();
	if (bAutoStart) StartProcessing();
	return true;
}

bool UCraftingQueueComponent::EnqueueByTag(FGameplayTag RecipeIDTag, int32 Quantity, AActor* Workstation)
{
	if (GetOwnerRole() != ROLE_Authority || !RecipeIDTag.IsValid() || Quantity < 1) return false;

	FCraftQueueEntry Entry;
	Entry.RecipeIDTag = RecipeIDTag;
	Entry.Quantity    = Quantity;
	Entry.JobId       = FGuid::NewGuid();
	Entry.StateTag    = TAG_State_Queued;
	Entry.ETASeconds  = 0.f;

	Queue.Add(Entry);
	FServerJob& SJ = ServerJobs.Add(Entry.JobId);
	SJ.Recipe      = nullptr; // resolve later if you add a recipe asset manager
	SJ.Workstation = Workstation;

	OnQueueChanged.Broadcast();
	if (bAutoStart) StartProcessing();
	return true;
}

bool UCraftingQueueComponent::CancelAtIndex(int32 Index)
{
	if (GetOwnerRole() != ROLE_Authority) return false;
	if (!Queue.IsValidIndex(Index)) return false;

	FCraftQueueEntry& E = Queue[Index];
	if (E.StateTag == TAG_State_InProgress) return false;

	E.StateTag = TAG_State_Cancelled;
	ServerJobs.Remove(E.JobId);
	Queue.RemoveAt(Index);
	OnQueueChanged.Broadcast();
	return true;
}

void UCraftingQueueComponent::ClearQueued()
{
	if (GetOwnerRole() != ROLE_Authority) return;
	for (int32 i = Queue.Num() - 1; i >= 0; --i)
	{
		if (Queue[i].StateTag == TAG_State_Queued)
		{
			ServerJobs.Remove(Queue[i].JobId);
			Queue.RemoveAt(i);
		}
	}
	OnQueueChanged.Broadcast();
}

void UCraftingQueueComponent::StartProcessing()
{
	if (GetOwnerRole() != ROLE_Authority) return;
	TryStartNext();
}

bool UCraftingQueueComponent::CanStartMore() const
{
	return InFlight.Num() < FMath::Max(1, MaxConcurrent);
}

void UCraftingQueueComponent::TryStartNext()
{
	if (!CanStartMore()) return;

	for (FCraftQueueEntry& E : Queue)
	{
		if (E.StateTag == TAG_State_Queued)
		{
			if (const FServerJob* SJ = ServerJobs.Find(E.JobId))
			{
				StartJob(E, *SJ);
				if (!CanStartMore()) break;
			}
		}
	}
}

void UCraftingQueueComponent::StartJob(FCraftQueueEntry& Entry, const FServerJob& SJ)
{
	UCraftingRecipeDataAsset* Recipe = SJ.Recipe.Get();
	if (!Recipe) return; // keep simple for MVP

	Entry.StateTag = TAG_State_InProgress;
	InFlight.Add(Entry.JobId);

	OnJobStarted.Broadcast(Entry.JobId, Entry.RecipeIDTag);
	OnQueueChanged.Broadcast();

	FGameplayEventData Data;
	Data.EventTag        = TAG_Event_Craft_Start;
	Data.OptionalObject  = Recipe;
	Data.OptionalObject2 = SJ.Workstation.Get();
	Data.EventMagnitude  = Entry.Quantity;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), Data.EventTag, Data);
}

void UCraftingQueueComponent::BindASCEvents()
{
	if (CachedASC.IsValid()) return;
	if (AActor* Owner = GetOwner())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
		{
			CachedASC = ASC;
			ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Craft_Started)
				.AddUObject(this, &UCraftingQueueComponent::OnCraftStartedEvent);
			ASC->GenericGameplayEventCallbacks.FindOrAdd(TAG_Event_Craft_Completed)
				.AddUObject(this, &UCraftingQueueComponent::OnCraftCompletedEvent);
		}
	}
}

void UCraftingQueueComponent::OnCraftStartedEvent(const FGameplayEventData* /*EventData*/)
{
	// Optional: update ETAs when GA sends predicted duration.
}

void UCraftingQueueComponent::OnCraftCompletedEvent(const FGameplayEventData* EventData)
{
	if (GetOwnerRole() != ROLE_Authority || !EventData) return;

	// We stored the recipe tag in TargetTags when the GA completed.
	FGameplayTag RecipeID;
	for (auto It = EventData->TargetTags.CreateConstIterator(); It; ++It)
	{
		const FGameplayTag T = *It;
		if (!T.MatchesTag(TAG_Event_Craft_Completed) && !T.MatchesTag(TAG_Event_Craft_Started))
		{
			RecipeID = T; break;
		}
	}

	int32 Found = INDEX_NONE;
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		if (Queue[i].StateTag == TAG_State_InProgress &&
			(!RecipeID.IsValid() || Queue[i].RecipeIDTag == RecipeID))
		{
			Found = i; break;
		}
	}
	if (Found == INDEX_NONE) return;

	FCraftQueueEntry E = Queue[Found];
	InFlight.Remove(E.JobId);
	E.StateTag = TAG_State_Completed;

	OnJobFinished.Broadcast(E.JobId, E.RecipeIDTag);

	ServerJobs.Remove(E.JobId);
	Queue.RemoveAt(Found);
	OnQueueChanged.Broadcast();

	TryStartNext();
}
