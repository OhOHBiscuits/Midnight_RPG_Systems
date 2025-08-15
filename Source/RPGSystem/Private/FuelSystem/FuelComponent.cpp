#include "FuelSystem/FuelComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "TimerManager.h"

UFuelComponent::UFuelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFuelComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFuelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BurnFuelTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UFuelComponent::StartBurn()
{
	if (!HasAuth() || bIsBurning || !FuelInventory) return;
	if (!HasFuel()) return;

	bIsBurning = true;
	TryStartNextFuel();
	OnBurnStarted.Broadcast();
}

void UFuelComponent::StopBurn()
{
	if (!HasAuth()) return;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BurnFuelTimer);
	}
	bIsBurning = false;
	OnBurnStopped.Broadcast();
	NotifyFuelStateChanged();
}

void UFuelComponent::PauseBurn()
{
	if (!HasAuth()) return;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().PauseTimer(BurnFuelTimer);
	}
}

void UFuelComponent::ResumeBurn()
{
	if (!HasAuth()) return;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().UnPauseTimer(BurnFuelTimer);
	}
}

bool UFuelComponent::HasFuel() const
{
	if (!FuelInventory) return false;
	for (const auto& Slot : FuelInventory->GetItems())
	{
		if (Slot.IsValid()) return true;
	}
	return false;
}

// Key fix: only keep burning if we have fuel AND (either we're not auto-idle-gating OR crafting is active)
bool UFuelComponent::ShouldKeepBurning() const
{
	const bool bHasFuel = HasFuel();
	const bool bActiveOk = (!bAutoStopBurnWhenIdle) || IsCraftingActive();
	return bHasFuel && bActiveOk;
}

void UFuelComponent::TryStartNextFuel()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& FuelItem = Items[i];
		if (!FuelItem.IsValid()) continue;

		if (UItemDataAsset* BurnedFuel = FuelItem.ResolveItemData())
		{
			const float FuelBurnTime = BurnedFuel->GetTotalBurnSeconds();
			TotalBurnTime     = FuelBurnTime / FMath::Max(0.01f, BurnSpeedMultiplier);
			RemainingBurnTime = TotalBurnTime;

			if (UWorld* W = GetWorld())
			{
				W->GetTimerManager().SetTimer(BurnFuelTimer, this, &UFuelComponent::BurnTimerTick, 1.0f, true);
			}
			NotifyFuelStateChanged();
			return;
		}
	}

	// No next fuel found — ensure a clean stop
	TotalBurnTime = 0.0f;
	RemainingBurnTime = 0.0f;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BurnFuelTimer);
	}
	DoAutoStop(); // sets bIsBurning=false, broadcasts OnBurnStopped
}

void UFuelComponent::BurnTimerTick()
{
	if (!HasAuth())
	{
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(BurnFuelTimer);
		}
		return;
	}

	if (RemainingBurnTime > 0.0f)
	{
		const float Delta = 1.0f * BurnSpeedMultiplier;
		RemainingBurnTime = FMath::Max(0.0f, RemainingBurnTime - Delta);
		OnFuelBurnProgress.Broadcast(RemainingBurnTime);

		if (RemainingBurnTime <= 0.0f)
		{
			if (UWorld* W = GetWorld())
			{
				W->GetTimerManager().ClearTimer(BurnFuelTimer);
			}

			BurnFuelOnce();
			OnFuelDepleted.Broadcast();

			if (ShouldKeepBurning())
			{
				TryStartNextFuel();
			}
			else
			{
				DoAutoStop();
			}
		}
	}
	else
	{
		// Safety path if tick fires with zero remaining
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(BurnFuelTimer);
		}
		DoAutoStop();
	}
}

void UFuelComponent::BurnFuelOnce()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& FuelItem = Items[i];
		if (!FuelItem.IsValid()) continue;

		UItemDataAsset* BurnedFuel = FuelItem.ResolveItemData();
		if (FuelInventory->TryRemoveItem(i, 1))
		{
			if (ByproductInventory && BurnedFuel)
			{
				for (const auto& By : BurnedFuel->FuelByproducts)
				{
					if (By.ByproductItemID.IsValid() && By.Amount > 0)
					{
						if (UItemDataAsset* ByAsset = UInventoryHelpers::FindItemDataByTag(this, By.ByproductItemID))
						{
							ByproductInventory->TryAddItem(ByAsset, By.Amount);
						}
					}
				}
			}
			LastBurnTime = GetWorld()->GetTimeSeconds();
		}
		break;
	}
}

void UFuelComponent::NotifyFuelStateChanged()
{
	// Reserved for UI/GAS hooks
}

void UFuelComponent::OnCraftingActivated()
{
	// Expansion point
}

void UFuelComponent::DoAutoStop()
{
	if (!bIsBurning) // prevent double-broadcast if multiple paths hit
	{
		TotalBurnTime = 0.0f;
		RemainingBurnTime = 0.0f;
		OnBurnStopped.Broadcast();
	}
	bIsBurning = false;
	NotifyFuelStateChanged();
}
