// CraftingStationComponent.cpp
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Net/UnrealNetwork.h"

#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

// If you want to wire real movement, include your inventory classes and replace the stubs below.
// #include "Inventory/InventoryComponent.h"

UCraftingStationComponent::UCraftingStationComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCraftingStationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCraftingStationComponent, ActiveJob);
	DOREPLIFETIME(UCraftingStationComponent, bIsCrafting);
	DOREPLIFETIME(UCraftingStationComponent, bIsPaused);
}

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe, int32 Times)
{
	if (bIsCrafting || !Recipe || Times <= 0)
	{
		return false;
	}

	// Stage materials first so we fail early if not enough.
	if (!MoveRequiredToInput(Recipe, Times))
	{
		return false;
	}

	// Populate the job
	ActiveJob = FCraftingJob{};
	ActiveJob.Recipe     = const_cast<UCraftingRecipeDataAsset*>(Recipe);
	ActiveJob.Instigator = InstigatorActor;
	ActiveJob.Count      = Times;
	ActiveJob.StartTime  = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Use a per-recipe duration variable on your data asset (or a constant for now).
	const float OneCraftDuration = 0.01f; // Replace with Recipe->CraftDurationSeconds if you have it
	const float TotalDuration    = FMath::Max(0.01f, OneCraftDuration * Times);
	ActiveJob.EndTime            = ActiveJob.StartTime + TotalDuration;

	bIsCrafting = true;
	bIsPaused   = false;
	OnCraftStarted.Broadcast(ActiveJob);

	// Start timer to call FinishCraft_Internal once the batch completes.
	GetWorld()->GetTimerManager().SetTimer(
		CraftTimerHandle,
		this, &UCraftingStationComponent::FinishCraft_Internal,
		TotalDuration,
		/*bLoop*/false
	);

	return true;
}

void UCraftingStationComponent::CancelCraft()
{
	if (!bIsCrafting)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
	// (Optional) Return staged materials from InputInventory here.
	bIsCrafting = false;
	bIsPaused   = false;

	OnCraftFinished.Broadcast(ActiveJob, /*bSuccess*/false);
	ActiveJob = FCraftingJob{};
}

void UCraftingStationComponent::PauseCraft()
{
	if (!bIsCrafting || bIsPaused) return;

	bIsPaused = true;

	// Save remaining time
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Remaining = FMath::Max(0.f, ActiveJob.EndTime - Now);
	ActiveJob.EndTime = Now + Remaining; // keep logical end for UI

	// Stop current countdown
	GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
}

void UCraftingStationComponent::ResumeCraft()
{
	if (!bIsCrafting || !bIsPaused) return;

	bIsPaused = false;

	const float Now       = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float TimeLeft  = FMath::Max(0.01f, ActiveJob.EndTime - Now);

	GetWorld()->GetTimerManager().SetTimer(
		CraftTimerHandle,
		this, &UCraftingStationComponent::FinishCraft_Internal,
		TimeLeft,
		/*bLoop*/false
	);
}

void UCraftingStationComponent::FinishCraft_Internal()
{
	const bool bSuccess = (ActiveJob.Recipe != nullptr);

	if (bSuccess)
	{
		DeliverOutputs(ActiveJob.Recipe);
		GiveFinishXPIIfAny(ActiveJob.Recipe, ActiveJob.Instigator.Get(), true);
	}

	OnCraftFinished.Broadcast(ActiveJob, bSuccess);

	bIsCrafting = false;
	bIsPaused   = false;
	ActiveJob   = FCraftingJob{};
}

bool UCraftingStationComponent::MoveRequiredToInput(const UCraftingRecipeDataAsset* /*Recipe*/, int32 /*Times*/)
{
	// ---- Replace this stub with your inventory logic. ----
	// The idea:
	// 1) Locate source inventory (e.g., on Instigator or the owner).
	// 2) Count if all required items (Times * each input) exist.
	// 3) Move to InputInventory so they’re reserved for the craft.
	//
	// Returning true here keeps things compiling & unblocked.
	return true;
}

void UCraftingStationComponent::DeliverOutputs(const UCraftingRecipeDataAsset* /*Recipe*/)
{
	// ---- Replace with your real inventory "add" API. ----
	// Iterate Recipe->Outputs and add items to OutputInventory.
}

void UCraftingStationComponent::GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* /*Recipe*/, AActor* /*InstigatorActor*/, bool /*bSuccess*/)
{
	// Hook to GAS / XP system if desired.
}

void UCraftingStationComponent::GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const
{
	Out.Reset();
	if (!Viewer) return;

	// 1) Viewer tags via interface
	if (IGameplayTagAssetInterface* GTAI = Cast<IGameplayTagAssetInterface>(Viewer))
	{
		GTAI->GetOwnedGameplayTags(Out);
	}

	// 2) Viewer ASC
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Viewer))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->GetOwnedGameplayTags(Out);
		}
	}

	// 3) If pawn, also check PS + PC
	if (const APawn* Pawn = Cast<APawn>(Viewer))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (IGameplayTagAssetInterface* PSTags = Cast<IGameplayTagAssetInterface>(PS))
			{
				PSTags->GetOwnedGameplayTags(Out);
			}
			if (const IAbilitySystemInterface* ASIPS = Cast<IAbilitySystemInterface>(PS))
			{
				if (UAbilitySystemComponent* ASC = ASIPS->GetAbilitySystemComponent())
				{
					ASC->GetOwnedGameplayTags(Out);
				}
			}
		}

		if (AController* PC = Pawn->GetController())
		{
			if (IGameplayTagAssetInterface* PCTags = Cast<IGameplayTagAssetInterface>(PC))
			{
				PCTags->GetOwnedGameplayTags(Out);
			}
			if (const IAbilitySystemInterface* ASIPC = Cast<IAbilitySystemInterface>(PC))
			{
				if (UAbilitySystemComponent* ASC = ASIPC->GetAbilitySystemComponent())
				{
					ASC->GetOwnedGameplayTags(Out);
				}
			}
		}
	}
}
