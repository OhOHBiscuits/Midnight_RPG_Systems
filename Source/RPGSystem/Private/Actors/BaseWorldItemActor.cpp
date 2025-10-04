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

void ABaseWorldItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseWorldItemActor, ItemData);
	DOREPLIFETIME(ABaseWorldItemActor, bUseEfficiency);
	DOREPLIFETIME(ABaseWorldItemActor, EfficiencyRating);
}

void ABaseWorldItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyItemDataVisuals();
}

#if WITH_EDITOR
void ABaseWorldItemActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ABaseWorldItemActor, ItemData))
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
		if (bUseEfficiency)
		{
			EfficiencyRating = Data->EfficiencyRating;
		}
	}
}

void ABaseWorldItemActor::Interact_Implementation(AActor* Interactor)
{
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

void ABaseWorldItemActor::HandleInteract_Server(AActor* /*Interactor*/)
{
	// Base does nothing; override in children (pickup/storage/workstation/weapon)
}

void ABaseWorldItemActor::ResolveOwningPCForUI(AActor* Interactor, APlayerController*& OutPC) const
{
	OutPC = nullptr;

	if (const APawn* Pawn = Cast<APawn>(Interactor))
	{
		OutPC = Cast<APlayerController>(Pawn->GetController());
	}
	else
	{
		OutPC = Cast<APlayerController>(Interactor);
	}

	if (!OutPC)
	{
		OutPC = UGameplayStatics::GetPlayerController(this, 0);
	}
}

void ABaseWorldItemActor::Client_ShowWorldItemUI_Implementation(AActor* Interactor, TSubclassOf<UUserWidget> ClassToUse)
{
	if (!ClassToUse) return;

	APlayerController* PC = nullptr;
	ResolveOwningPCForUI(Interactor, PC);

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

	if (HasAuthority())
	{
		AController* OwnerCtrl = nullptr;
		if (APawn* P = Cast<APawn>(Interactor)) OwnerCtrl = P->GetController();
		if (!OwnerCtrl) OwnerCtrl = Cast<AController>(Interactor);
		if (OwnerCtrl) { SetOwner(OwnerCtrl); }

		Client_ShowWorldItemUI(Interactor, ClassToUse);
	}
	else
	{
		Client_ShowWorldItemUI(Interactor, ClassToUse);
	}
}
