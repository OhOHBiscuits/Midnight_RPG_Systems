// CraftingStationComponent.cpp
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Actors/WorkstationActor.h"

#include "Net/UnrealNetwork.h"
#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

// If you actually grant items here, include your inventory API and use it in DeliverOutputs
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
}

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* InstigatorActor, const UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe) return false;

	// If the owner is a workstation and uses a data asset, enforce the allow-list
	if (const AWorkstationActor* WS = Cast<AWorkstationActor>(GetOwner()))
	{
		if (!WS->IsRecipeAllowed(Recipe))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Crafting] Recipe %s not allowed by station."), *GetNameSafe(Recipe));
			return false;
		}
	}

	// … your existing job setup (ActiveJob = …, timers, broadcast, etc.) …
	return true;
}

void UCraftingStationComponent::CancelCraft()
{
	if (!bIsCrafting) return;

	GetWorld()->GetTimerManager().ClearTimer(CraftTimerHandle);
	FinishCraft_Internal(false);
}

void UCraftingStationComponent::FinishCraft_Internal(bool bSuccess)
{
	// Deliver outputs / XP only when we have valid job data and success.
	if (ActiveJob.Recipe && ActiveJob.Instigator.IsValid() && bSuccess)
	{
		DeliverOutputs(ActiveJob.Recipe, ActiveJob.Instigator.Get());
		GiveFinishXPIIfAny(ActiveJob.Recipe, ActiveJob.Instigator.Get(), true);
	}

	OnCraftFinished.Broadcast(ActiveJob, bSuccess);

	bIsCrafting = false;
	ActiveJob   = FCraftingJob{};
}

void UCraftingStationComponent::DeliverOutputs(const UCraftingRecipeDataAsset* Recipe, AActor* InstigatorActor)
{
	if (!Recipe) return;

	// Example (pseudo): iterate outputs. Adjust to your real type names.
	// If your outputs type is FCraftItemOutput, use that; if FCraftingItemQuantity, swap accordingly.
	/*
	for (const FCraftItemOutput& Out : Recipe->Outputs)
	{
		if (!Out.Item) continue;

		// Find an inventory on the owner or wherever you want to send items.
		UInventoryComponent* OutInv = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
		if (OutInv)
		{
			// Replace with your real API:
			// OutInv->AddItemByAsset(Out.Item, Out.Quantity);
		}
	}
	*/
}

void UCraftingStationComponent::GiveFinishXPIIfAny(const UCraftingRecipeDataAsset* /*Recipe*/, AActor* /*InstigatorActor*/, bool /*bSuccess*/)
{
	// Wire to your XP system later (GAS effect, ability, or custom grant).
}

void UCraftingStationComponent::GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out) const
{
	Out.Reset();
	if (!Viewer) return;

	// 1) Tags on the actor itself (interface)
	if (IGameplayTagAssetInterface* GTAI = Cast<IGameplayTagAssetInterface>(Viewer))
	{
		GTAI->GetOwnedGameplayTags(Out);
	}

	// 2) Tags via the actor's ASC
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Viewer))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->GetOwnedGameplayTags(Out);
		}
	}

	// 3) If it's a pawn, also check PlayerState and Controller
	if (const APawn* Pawn = Cast<APawn>(Viewer))
	{
		// PlayerState
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

		// Controller
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

void UCraftingStationComponent::AddRecipe(UCraftingRecipeDataAsset* Recipe)
{
	if (Recipe)
	{
		StationRecipes.AddUnique(Recipe);
	}
}

void UCraftingStationComponent::RemoveRecipe(UCraftingRecipeDataAsset* Recipe)
{
	StationRecipes.Remove(Recipe);
}

bool UCraftingStationComponent::HasRecipe(const UCraftingRecipeDataAsset* Recipe) const
{
	return StationRecipes.Contains(const_cast<UCraftingRecipeDataAsset*>(Recipe));
}

