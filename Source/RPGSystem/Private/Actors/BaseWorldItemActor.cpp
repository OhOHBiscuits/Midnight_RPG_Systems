// BaseWorldItemActor.cpp
#include "Actors/BaseWorldItemActor.h"
#include "Inventory/ItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ABaseWorldItemActor::ABaseWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetIsReplicated(true);
}

void ABaseWorldItemActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWorldItemActor, ItemData);
	DOREPLIFETIME(ABaseWorldItemActor, bUseEfficiency);
	DOREPLIFETIME(ABaseWorldItemActor, EfficiencyRating);
}

void ABaseWorldItemActor::OnConstruction(const FTransform& Xform)
{
	Super::OnConstruction(Xform);
	ApplyItemDataVisuals();
}

#if WITH_EDITOR
void ABaseWorldItemActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	if (Event.Property && Event.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ABaseWorldItemActor, ItemData))
	{
		ApplyItemDataVisuals();
	}
}
#endif

void ABaseWorldItemActor::OnRep_ItemData()
{
	ApplyItemDataVisuals();
}

void ABaseWorldItemActor::ApplyItemDataVisuals()
{
	if (!Mesh) return;

	if (ItemData.IsNull())
	{
		Mesh->SetStaticMesh(nullptr);
		return;
	}

	if (UItemDataAsset* Data = ItemData.LoadSynchronous())
	{
		if (UStaticMesh* M = Data->GetWorldMeshSync())
		{
			Mesh->SetStaticMesh(M);
		}
		else
		{
			Mesh->SetStaticMesh(nullptr);
		}

		// Optionally mirror a default efficiency from the data
		EfficiencyRating = Data->EfficiencyRating;
	}
}

void ABaseWorldItemActor::Interact_Implementation(AActor* Interactor)
{
	// Always route to the authoritative server
	if (HasAuthority())
	{
		HandleInteract_Server(Interactor);
	}
	else
	{
		Server_Interact(Interactor);
	}
}

void ABaseWorldItemActor::Server_Interact_Implementation(AActor* Interactor)
{
	HandleInteract_Server(Interactor);
}

void ABaseWorldItemActor::Client_ShowWorldItemUI_Implementation(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse)
{
	if (!ClassToUse) return;

	APlayerController* PC = nullptr;

	if (APawn* Pawn = Cast<APawn>(Interactor))
	{
		PC = Cast<APlayerController>(Pawn->GetController());
	}
	else
	{
		PC = Cast<APlayerController>(Interactor);
	}
	if (!PC) PC = UGameplayStatics::GetPlayerController(this, 0);

	if (PC && PC->IsLocalController())
	{
		if (UUserWidget* W = CreateWidget<UUserWidget>(PC, ClassToUse))
		{
			W->AddToViewport();
		}
	}
}

void ABaseWorldItemActor::ShowWorldItemUI(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse)
{
	if (!ClassToUse) return;

	if (GetLocalRole() == ROLE_Authority)
	{
		// make RPC reach the right client
		if (APawn* Pawn = Cast<APawn>(Interactor))
		{
			if (AController* C = Pawn->GetController())
			{
				SetOwner(C);
			}
		}
		else if (AController* AsCtrl = Cast<AController>(Interactor))
		{
			SetOwner(AsCtrl);
		}
		Client_ShowWorldItemUI(Interactor, ClassToUse);
	}
	else
	{
		Client_ShowWorldItemUI(Interactor, ClassToUse);
	}
}
