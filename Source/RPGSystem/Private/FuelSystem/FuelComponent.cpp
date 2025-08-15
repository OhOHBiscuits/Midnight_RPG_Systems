#include "FuelSystem/FuelComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryHelpers.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UFuelComponent::UFuelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Make the component itself replicate (it’s still the owning actor that carries it across the network)
	SetIsReplicatedByDefault(true);
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

/* ------------ Replication ------------ */

void UFuelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFuelComponent, TotalBurnTime);
	DOREPLIFETIME(UFuelComponent, RemainingBurnTime);
	DOREPLIFETIME(UFuelComponent, bIsBurning);
}

void UFuelComponent::OnRep_FuelState()
{
	// Keep UI in sync on clients
	OnFuelBurnProgress.Broadcast(RemainingBurnTime);
	if (bIsBurning) OnBurnStarted.Broadcast(); else OnBurnStopped.Broadcast();
}

/* ------------ Public API (one-call from server or client) ------------ */

void UFuelComponent::StartBurn()
{
	if (HasAuth())  StartBurn_ServerImpl();
	else            Server_StartBurn();
}

void UFuelComponent::StopBurn()
{
	if (HasAuth())  StopBurn_ServerImpl();
	else            Server_StopBurn();
}

void UFuelComponent::PauseBurn()
{
	if (HasAuth())  PauseBurn_ServerImpl();
	else            Server_PauseBurn();
}

void UFuelComponent::ResumeBurn()
{
	if (HasAuth())  ResumeBurn_ServerImpl();
	else            Server_ResumeBurn();
}

/* ------------ Client->Server RPCs ------------ */

void UFuelComponent::Server_StartBurn_Implementation()  { StartBurn_ServerImpl(); }
void UFuelComponent::Server_StopBurn_Implementation()   { StopBurn_ServerImpl(); }
void UFuelComponent::Server_PauseBurn_Implementation()  { PauseBurn_ServerImpl(); }
void UFuelComponent::Server_ResumeBurn_Implementation() { ResumeBurn_ServerImpl(); }

/* ------------ Server implementations ------------ */

void UFuelComponent::StartBurn_ServerImpl()
{
	if (bIsBurning || !FuelInventory) return;
	if (!HasFuel()) return;

	bIsBurning = true;
	TryStartNextFuel();
	OnBurnStarted.Broadcast();
	NotifyFuelStateChanged();
}

void UFuelComponent::StopBurn_ServerImpl()
{
	bIsBurning = false;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BurnFuelTimer);
	}
	OnBurnStopped.Broadcast();
	NotifyFuelStateChanged();
}

void UFuelComponent::PauseBurn_ServerImpl()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().PauseTimer(BurnFuelTimer);
	}
}

void UFuelComponent::ResumeBurn_ServerImpl()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().UnPauseTimer(BurnFuelTimer);
	}
}

/* ------------ Core logic (server) ------------ */

bool UFuelComponent::HasFuel() const
{
	if (!FuelInventory) return false;
	const TArray<FInventoryItem>& Items = FuelInventory->GetItem();
	for (const FInventoryItem& Item : Items)
	{
		if (Item.IsValid())
			return true;
	}
	return false;
}

bool UFuelComponent::ShouldKeepBurning() const
{
	return bAutoStopBurnWhenIdle ? IsCraftingActive() : HasFuel();
}

void UFuelComponent::TryStartNextFuel()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItem();
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

				if (UWorld* W = GetWorld())
				{
					W->GetTimerManager().SetTimer(BurnFuelTimer, this, &UFuelComponent::BurnTimerTick, 1.0f, true);
				}
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
				bIsBurning = false;
				NotifyFuelStateChanged();
			}
		}
	}
	else
	{
		if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().ClearTimer(BurnFuelTimer);
		}
		bIsBurning = false;
		NotifyFuelStateChanged();
	}
}

void UFuelComponent::BurnFuelOnce()
{
	if (!HasAuth() || !FuelInventory) return;

	const TArray<FInventoryItem>& Items = FuelInventory->GetItem();
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
	// Reserved for UI hooks or GAS gameplay cues
}

void UFuelComponent::OnCraftingActivated()
{
	// Expansion point
}
