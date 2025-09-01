#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UCraftingStationComponent::UCraftingStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingStationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCraftingStationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);
	Super::EndPlay(EndPlayReason);
}

/* ------------------------ Query helpers ------------------------ */

void UCraftingStationComponent::GetAllStationRecipes(TArray<UCraftingRecipeDataAsset*>& Out) const
{
	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : StationRecipes)
	{
		if (UCraftingRecipeDataAsset* R = Soft.LoadSynchronous())
		{
			if (PassesStationFilters(R))
			{
				Out.Add(R);
			}
		}
	}
	// If you want to support AllowedRecipeTags-driven discovery, add your registry/manager lookup here.
}

void UCraftingStationComponent::GetCraftableRecipes(AActor* Viewer, TArray<UCraftingRecipeDataAsset*>& OutCraftable, TArray<UCraftingRecipeDataAsset*>& OutLocked) const
{
	TArray<UCraftingRecipeDataAsset*> All; GetAllStationRecipes(All);
	for (UCraftingRecipeDataAsset* R : All)
	{
		if (!R) continue;
		if (IsRecipeUnlocked(Viewer, R))
		{
			OutCraftable.Add(R);
		}
		else
		{
			OutLocked.Add(R);
		}
	}
}

/* ------------------------ Entry points ------------------------ */

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* Instigator, UCraftingRecipeDataAsset* Recipe, int32 Count)
{
	if (!Instigator || !Recipe) return false;
	if (!PassesStationFilters(Recipe)) return false;
	if (!IsRecipeUnlocked(Instigator, Recipe)) return false;

	// Try to consume inputs up front
	if (!HasAndConsumeInputs(Instigator, Recipe, Count))
	{
		return false;
	}

	const float totalTime = FMath::Max(0.f, Recipe->BaseTimeSeconds * Count);

	if (!bProcessing && (totalTime <= InstantThresholdSeconds || !bEnableQueue))
	{
		// Instant or direct craft
		FCraftingJob Job; Job.RecipeID = Recipe->RecipeIDTag; Job.Recipe = Recipe; Job.Count = Count; Job.TotalTime = totalTime; Job.TimeRemaining = totalTime; Job.Instigator = Instigator;
		OnCraftStarted.Broadcast(Job);

		if (totalTime <= 0.f)
		{
			GrantOutputsAndXP(Instigator, Recipe, Count, /*bSuccess*/true);
			OnCraftFinished.Broadcast(Job, /*bSuccess*/true);
		}
		else
		{
			bProcessing = true;
			Queue.Insert(Job, 0);
			GetWorld()->GetTimerManager().SetTimer(CraftTimer, this, &UCraftingStationComponent::TickActive, 0.1f, true);
		}
		return true;
	}

	// Enqueue
	Enqueue(Recipe, Instigator, Count);
	return true;
}

bool UCraftingStationComponent::StartCraftByTag(AActor* Instigator, FGameplayTag RecipeID, int32 Count)
{
	const UCraftingRecipeDataAsset* Recipe = nullptr;
	if (!ResolveRecipeByTag(RecipeID, Recipe)) return false;
	return StartCraftFromRecipe(Instigator, const_cast<UCraftingRecipeDataAsset*>(Recipe), Count);
}

void UCraftingStationComponent::CancelActive()
{
	if (!bProcessing) return;
	FinishActive(/*bSuccess*/false);
}

/* ------------------------ Internals ------------------------ */

bool UCraftingStationComponent::ResolveRecipeByTag(FGameplayTag RecipeID, const UCraftingRecipeDataAsset*& OutRecipe) const
{
	for (const TSoftObjectPtr<UCraftingRecipeDataAsset>& Soft : StationRecipes)
	{
		if (const UCraftingRecipeDataAsset* R = Soft.LoadSynchronous())
		{
			if (R->RecipeIDTag == RecipeID) { OutRecipe = R; return true; }
		}
	}
	return false;
}

bool UCraftingStationComponent::PassesStationFilters(const UCraftingRecipeDataAsset* Recipe) const
{
	if (!Recipe) return false;
	if (StationDiscipline.IsValid() && Recipe->CraftingDiscipline.IsValid() && Recipe->CraftingDiscipline != StationDiscipline)
	{
		return false;
	}
	if (!Recipe->RequiredStationTags.IsEmpty() && !StationTags.HasAll(Recipe->RequiredStationTags))
	{
		return false;
	}
	// Optional: AllowedRecipeTags filter (by RecipeIDTag)
	if (!AllowedRecipeTags.IsEmpty() && !AllowedRecipeTags.HasTag(Recipe->RecipeIDTag))
	{
		return false;
	}
	return true;
}

bool UCraftingStationComponent::IsRecipeUnlocked(AActor* Viewer, const UCraftingRecipeDataAsset* Recipe) const
{
	// If no unlock tag, treat as unlocked
	if (!Recipe || !Recipe->UnlockTag.IsValid()) return true;

	// Simple default: if the viewer (or its components) has the tag, it’s unlocked.
	// Replace with your own unlock system (ASC owned tags, save data, etc.)
	if (Viewer)
	{
		FGameplayTagContainer Owned;
		Viewer->GetOwnedGameplayTags(Owned);
		return Owned.HasTag(Recipe->UnlockTag);
	}
	return false;
}

bool UCraftingStationComponent::HasAndConsumeInputs(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe, int32 Count)
{
	// TODO: Hook to your inventory search: player + nearby/public chests.
	// For now return true so you can test flow quickly.
	return true;
}

void UCraftingStationComponent::GrantOutputsAndXP(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe, int32 Count, bool bSuccess)
{
	if (!Recipe || !Instigator) return;

	// TODO: Add items to output inventory / drop in world (use your InventoryComponent helpers)
	// TODO: Award skill XP via your ExecCalc / GE (Recipe->PrimarySkillData, Recipe->BaseSkillXP * Count)
	// Keep it server-authoritative.
}

void UCraftingStationComponent::Enqueue(const UCraftingRecipeDataAsset* Recipe, AActor* Instigator, int32 Count)
{
	FCraftingJob Job;
	Job.RecipeID = Recipe->RecipeIDTag;
	Job.Recipe   = Recipe;
	Job.Count    = Count;
	Job.TotalTime = FMath::Max(0.f, Recipe->BaseTimeSeconds * Count);
	Job.TimeRemaining = Job.TotalTime;
	Job.Instigator = Instigator;

	Queue.Add(Job);

	if (!bProcessing)
	{
		TryStartNext();
	}
}

void UCraftingStationComponent::TryStartNext()
{
	if (Queue.Num() == 0 || bProcessing) return;

	bProcessing = true;

	// Fire Started for the head
	OnCraftStarted.Broadcast(Queue[0]);

	// Tick at 0.1s – adjust as you like
	GetWorld()->GetTimerManager().SetTimer(CraftTimer, this, &UCraftingStationComponent::TickActive, 0.1f, true);
}

void UCraftingStationComponent::TickActive()
{
	if (Queue.Num() == 0) { GetWorld()->GetTimerManager().ClearTimer(CraftTimer); bProcessing = false; return; }

	FCraftingJob& Head = Queue[0];
	Head.TimeRemaining = FMath::Max(0.f, Head.TimeRemaining - 0.1f);

	if (Head.TimeRemaining <= 0.f)
	{
		FinishActive(/*bSuccess*/true);
	}
}

void UCraftingStationComponent::FinishActive(bool bSuccess)
{
	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);
	if (Queue.Num() == 0) { bProcessing = false; return; }

	FCraftingJob Done = Queue[0];
	Queue.RemoveAt(0);

	GrantOutputsAndXP(Done.Instigator.Get(), Done.Recipe, Done.Count, bSuccess);
	OnCraftFinished.Broadcast(Done, bSuccess);

	bProcessing = false;
	TryStartNext(); // continue with next
}
