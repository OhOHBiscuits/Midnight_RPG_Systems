#include "Actors/PickupItemActor.h"
#include "Actors/BaseWorldItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"

APickupItemActor::APickupItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void APickupItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickupItemActor, DecayTimeRemaining);
	DOREPLIFETIME(APickupItemActor, DecayState);
}

void APickupItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bEnableDecay && !ItemData.IsNull())
	{
		if (UItemDataAsset* Data = ItemData.LoadSynchronous())
		{
			if (Data->bCanDecay)
			{
				TotalDecayTime = Data->GetTotalDecaySeconds();
				DecayTimeRemaining = TotalDecayTime;
				StartDecayTimer();
			}
		}
	}
}

void APickupItemActor::StartDecayTimer()
{
	if (!HasAuthority() || TotalDecayTime <= 0.f) return;

	GetWorldTimerManager().SetTimer(
		DecayTimerHandle,
		this,
		&APickupItemActor::DecayTimerTick,
		1.0f,
		true
	);
}

void APickupItemActor::DecayTimerTick()
{
	if (!HasAuthority()) return;

	if (DecayTimeRemaining > 0.f)
	{
		DecayTimeRemaining -= 1.f;
	}
	else
	{
		HandleDecayComplete();
	}
}

void APickupItemActor::HandleDecayComplete()
{
	if (!HasAuthority()) return;

	GetWorldTimerManager().ClearTimer(DecayTimerHandle);
	DecayState = EPickupDecayState::Decayed;

	// Transform into decayed actor/pickup if specified
	if (UItemDataAsset* Data = ItemData.LoadSynchronous())
	{
		if (Data->DecaysIntoActorClass.ToSoftObjectPath().IsValid())
		{
			if (UClass* DecayedClass = Data->DecaysIntoActorClass.LoadSynchronous())
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				Params.Owner = this;
				Params.Instigator = GetInstigator();
				if (AActor* NewActor = GetWorld()->SpawnActor<AActor>(DecayedClass, GetActorLocation(), GetActorRotation(), Params))
				{
					if (APickupItemActor* NewPickup = Cast<APickupItemActor>(NewActor))
					{
						if (Data->DecaysInto.ToSoftObjectPath().IsValid())
						{
							if (UItemDataAsset* NewData = Data->DecaysInto.LoadSynchronous())
							{
								NewPickup->ItemData = NewData;
							}
						}
						NewPickup->Quantity = Quantity;
						NewPickup->bEnableDecay = false; // prevent infinite loops
					}
				}
			}
		}
	}

	OnPickupDecayed.Broadcast();
	Destroy();
}

void APickupItemActor::OnRep_DecayState()
{
	// VFX/SFX hook
}

void APickupItemActor::HandleInteract_Server(AActor* Interactor)
{
	if (ItemData.IsNull() || Quantity <= 0) return;

	UInventoryComponent* Inv = UInventoryHelpers::GetInventoryComponent(Interactor);
	if (!Inv) return;

	if (UItemDataAsset* Data = ItemData.LoadSynchronous())
	{
		if (Inv->TryAddItem(Data, Quantity))
		{
			ShowWorldItemUI(Interactor, PickupToastWidgetClass);
			Destroy();
		}
		else
		{
			// failed to add — could also show a "inventory full" toast
			ShowWorldItemUI(Interactor, PickupToastWidgetClass);
		}
	}
}
