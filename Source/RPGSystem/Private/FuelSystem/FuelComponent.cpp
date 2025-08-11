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
	GetWorld()->GetTimerManager().ClearTimer(BurnFuelTimer);
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
	bIsBurning = false;
	GetWorld()->GetTimerManager().ClearTimer(BurnFuelTimer);
	OnBurnStopped.Broadcast();
	NotifyFuelStateChanged();
}

void UFuelComponent::PauseBurn()
{
	if (!HasAuth()) return;
	GetWorld()->GetTimerManager().PauseTimer(BurnFuelTimer);
}

void UFuelComponent::ResumeBurn()
{
	if (!HasAuth()) return;
	GetWorld()->GetTimerManager().UnPauseTimer(BurnFuelTimer);
}

bool UFuelComponent::HasFuel() const
{
	if (!FuelInventory) return false;
	const TArray<FInventoryItem>& Items = FuelInventory->GetItems();
	for (const FInventoryItem& Item : Items)
	{
		if (Item.IsValid())
			return true;
	}
	return false;
}

bool UFuelComponent::ShouldKeepBurning() const
{
	if (bAutoStopBurnWhenIdle)
	{
		return IsCraftingActive();
	}
	return HasFuel();
}

void UFuelComponent::TryStartNextFuel()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& FuelItem = Items[i];
		if (FuelItem.IsValid())
		{
			if (UItemDataAsset* BurnedFuel = FuelItem.ResolveItemData())
			{
				const float FuelBurnTime = BurnedFuel->GetTotalBurnSeconds();
				TotalBurnTime      = FuelBurnTime / FMath::Max(0.01f, BurnSpeedMultiplier);
				RemainingBurnTime  = TotalBurnTime;

				GetWorld()->GetTimerManager().SetTimer(BurnFuelTimer, this, &UFuelComponent::BurnTimerTick, 1.0f, true);
				NotifyFuelStateChanged();
				return;
			}
		}
	}

	TotalBurnTime = 0.0f;
	RemainingBurnTime = 0.0f;
	NotifyFuelStateChanged();
}

void UFuelComponent::BurnTimerTick()
{
	if (!HasAuth())
	{
		GetWorld()->GetTimerManager().ClearTimer(BurnFuelTimer);
		return;
	}

	if (RemainingBurnTime > 0.0f)
	{
		const float Delta = 1.0f * BurnSpeedMultiplier;
		RemainingBurnTime = FMath::Max(0.0f, RemainingBurnTime - Delta);
		OnFuelBurnProgress.Broadcast(RemainingBurnTime);

		if (RemainingBurnTime <= 0.0f)
		{
			GetWorld()->GetTimerManager().ClearTimer(BurnFuelTimer);
			BurnFuelOnce();
			OnFuelDepleted.Broadcast();

			if (ShouldKeepBurning())
			{
				TryStartNextFuel();
			}
			else
			{
				bIsBurning = false;
				NotifyFuelStateChanged();
			}
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(BurnFuelTimer);
		bIsBurning = false;
		NotifyFuelStateChanged();
	}
}

void UFuelComponent::BurnFuelOnce()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FInventoryItem& FuelItem = Items[i];
		if (FuelItem.IsValid())
		{
			UItemDataAsset* BurnedFuel = FuelItem.ResolveItemData();
			if (FuelInventory->TryRemoveItem(i, 1))
			{
				if (ByproductInventory && BurnedFuel)
				{
					for (const FFuelByproduct& By : BurnedFuel->FuelByproducts)
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
}

void UFuelComponent::NotifyFuelStateChanged()
{
	// Reserved for UI hooks or GAS events
}

void UFuelComponent::OnCraftingActivated()
{
	// Expansion point
}
