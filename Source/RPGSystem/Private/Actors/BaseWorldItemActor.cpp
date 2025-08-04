#include "Actors/BaseWorldItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ABaseWorldItemActor::ABaseWorldItemActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetCollisionProfileName("BlockAll");
    RootComponent = MeshComp;

    bUseEfficiency = false; // Default is off, override in children as needed
}

void ABaseWorldItemActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaseWorldItemActor, EfficiencyRating);
}

void ABaseWorldItemActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SetMeshFromData();
}

void ABaseWorldItemActor::SetMeshFromData_Implementation()
{
    if (ItemData && ItemData->WorldMesh.IsValid())
    {
        MeshComp->SetStaticMesh(ItemData->WorldMesh.Get());
    }
}

void ABaseWorldItemActor::InitFromItemData(UItemDataAsset* NewItemData)
{
    ItemData = NewItemData;
    SetMeshFromData();

    // Only use efficiency if enabled!
    if (bUseEfficiency && ItemData && ItemData->EfficiencyRating > 0)
    {
        float OldEfficiency = EfficiencyRating;
        EfficiencyRating = ItemData->EfficiencyRating;

        if (EfficiencyRating != OldEfficiency)
        {
            OnRep_EfficiencyRating();
        }
    }
}

void ABaseWorldItemActor::OnRep_EfficiencyRating()
{
    OnEfficiencyChanged.Broadcast(EfficiencyRating);
}

void ABaseWorldItemActor::Interact_Implementation(AActor* Interactor)
{
    if (!HasAuthority())
    {
        ServerInteract(Interactor);
        return;
    }

    OnWorldItemInteracted.Broadcast(Interactor);
    // Extend in children for pickup/storage/crafting logic
}

void ABaseWorldItemActor::ServerInteract_Implementation(AActor* Interactor)
{
    Interact_Implementation(Interactor);
}

bool ABaseWorldItemActor::ServerInteract_Validate(AActor* Interactor)
{
    return true;
}

void ABaseWorldItemActor::OnFocused_Implementation() {}
void ABaseWorldItemActor::OnUnfocused_Implementation() {}

void ABaseWorldItemActor::TriggerWorldItemUI(AActor* Interactor)
{
    if (!Interactor)
        return;

    // Try to get the correct PlayerController (works for pawn, controller, etc)
    APlayerController* PC = nullptr;
    if (APawn* Pawn = Cast<APawn>(Interactor))
    {
        PC = Cast<APlayerController>(Pawn->GetController());
    }
    else
    {
        PC = Cast<APlayerController>(Interactor);
    }

    // If not a local controller, send to the owning client
    if (PC && !PC->IsLocalController())
    {
        Client_TriggerWorldItemUI(PC);
        return;
    }

    if (PC && PC->IsLocalController() && WidgetClass)
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
        if (Widget)
        {
            Widget->AddToViewport();
        }
    }
}

void ABaseWorldItemActor::Client_TriggerWorldItemUI_Implementation(AActor* Interactor)
{
    TriggerWorldItemUI(Interactor);
}

FGameplayTag ABaseWorldItemActor::GetInteractionTypeTag_Implementation() const
{
    return InteractionTypeTag;
}

