#include "Actors/PickupItemActor.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

APickupItemActor::APickupItemActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
}

void APickupItemActor::BeginPlay()
{
    Super::BeginPlay();

    // Decay only runs on the server
    if (HasAuthority() && bEnableDecay && ItemData && ItemData->bCanDecay)
    {
        TotalDecayTime = ItemData->GetTotalDecaySeconds();
        DecayTimeRemaining = TotalDecayTime;

        StartDecayTimer();
    }
}

void APickupItemActor::StartDecayTimer()
{
    if (TotalDecayTime > 0.f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            DecayTimerHandle,
            this,
            &APickupItemActor::DecayTimerTick,
            1.0f,
            true
        );
    }
}

void APickupItemActor::DecayTimerTick()
{
    // Server only
    if (!HasAuthority()) return;

    if (DecayTimeRemaining > 0.f)
    {
        DecayTimeRemaining -= 1.0f;
        // (Optional: Send progress to UI, will replicate to all clients)
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(DecayTimerHandle);
        HandleDecayComplete();
    }
}

void APickupItemActor::HandleDecayComplete()
{
    if (!HasAuthority()) return;

    // If you use DecaysIntoActorClass for world decay:
    if (ItemData && ItemData->DecaysIntoActorClass.ToSoftObjectPath().IsValid())
    {
        UClass* DecayedClass = ItemData->DecaysIntoActorClass.LoadSynchronous();
        if (DecayedClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            SpawnParams.Owner = this;
            SpawnParams.Instigator = GetInstigator();

            AActor* NewActor = GetWorld()->SpawnActor<AActor>(DecayedClass, GetActorLocation(), GetActorRotation(), SpawnParams);

            if (APickupItemActor* NewPickup = Cast<APickupItemActor>(NewActor))
            {
                // If you also want to set the decayed item asset on the new pickup:
                if (ItemData->DecaysInto.ToSoftObjectPath().IsValid())
                {
                    UItemDataAsset* DecayedItemAsset = ItemData->DecaysInto.LoadSynchronous();
                    if (DecayedItemAsset)
                    {
                        NewPickup->ItemData = DecayedItemAsset;
                    }
                }
                NewPickup->Quantity = Quantity;
                // Optional: stop further decay, or leave true if you want multi-step decay chains
                NewPickup->bEnableDecay = false;
            }
        }
    }

    OnPickupDecayed.Broadcast();
    Destroy(); // server destroys; replicas vanish for clients
}



void APickupItemActor::OnRep_DecayState()
{
    if (DecayState == EPickupDecayState::Decayed)
    {
        // Play mesh swap, rot VFX, etc in Blueprint or C++
        // (Optional: do nothing if item is immediately destroyed on server)
    }
}

void APickupItemActor::Interact_Implementation(AActor* Interactor)
{
    Super::Interact_Implementation(Interactor);
    if (!HasAuthority())
        return;

    if (!ItemData || Quantity <= 0 || !Interactor) return;

    UInventoryComponent* Inv = UInventoryHelpers::GetInventoryComponent(Interactor);
    if (Inv && Inv->TryAddItem(ItemData, Quantity))
    {
        Destroy();
    }
    else
    {
        TriggerWorldItemUI(Interactor);
    }
}

void APickupItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APickupItemActor, DecayTimeRemaining);
    DOREPLIFETIME(APickupItemActor, DecayState);
}
