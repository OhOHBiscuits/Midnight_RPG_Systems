// InventoryHelpers.cpp
#include "Inventory/InventoryHelpers.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryAssetManager.h"

#include "Kismet/GameplayStatics.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

static UInventoryComponent* FindInvOnActor(const AActor* Actor)
{
	if (!Actor) return nullptr;

	// 1) Directly on the actor
	if (const UActorComponent* C = Actor->GetComponentByClass(UInventoryComponent::StaticClass()))
	{
		return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C));
	}

	// 2) Common places for player-owned inventories
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		// On Pawn
		if (const UActorComponent* C2 = Pawn->GetComponentByClass(UInventoryComponent::StaticClass()))
			return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C2));

		// On PlayerState
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			if (const UActorComponent* C3 = PS->GetComponentByClass(UInventoryComponent::StaticClass()))
				return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C3));
		}

		// On Controller
		if (const AController* Ctrl = Pawn->GetController())
		{
			if (const UActorComponent* C4 = Ctrl->GetComponentByClass(UInventoryComponent::StaticClass()))
				return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C4));
		}
	}

	// 3) If it's already a Controller
	if (const AController* Ctrl = Cast<AController>(Actor))
	{
		if (const UActorComponent* C5 = Ctrl->GetComponentByClass(UInventoryComponent::StaticClass()))
			return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C5));

		// Check possessed pawn
		if (const APawn* Pawn2 = Ctrl->GetPawn())
		{
			if (const UActorComponent* C6 = Pawn2->GetComponentByClass(UInventoryComponent::StaticClass()))
				return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C6));
		}

		// Check PlayerState
		if (const APlayerState* PS2 = Ctrl->PlayerState)
		{
			if (const UActorComponent* C7 = PS2->GetComponentByClass(UInventoryComponent::StaticClass()))
				return Cast<UInventoryComponent>(const_cast<UActorComponent*>(C7));
		}
	}

	return nullptr;
}

UInventoryComponent* UInventoryHelpers::GetInventoryComponent(AActor* Actor)
{
	return FindInvOnActor(Actor);
}

UInventoryComponent* UInventoryHelpers::GetInventoryComponentFromObject(const UObject* Object)
{
	if (!Object) return nullptr;

	// If it's an actor or derives from it, scan likely owners
	if (const AActor* AsActor = Cast<AActor>(Object))
	{
		if (UInventoryComponent* Inv = FindInvOnActor(AsActor)) return Inv;

		// Owner chain
		if (const AActor* Owner = AsActor->GetOwner())
			if (UInventoryComponent* Inv2 = FindInvOnActor(Owner)) return Inv2;
	}

	// If it's a component, try its owner
	if (const UActorComponent* Comp = Cast<UActorComponent>(Object))
	{
		if (const AActor* Owner = Comp->GetOwner())
			if (UInventoryComponent* Inv3 = FindInvOnActor(Owner)) return Inv3;
	}

	// Last resort: try player 0 pawn
	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Object, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			return FindInvOnActor(PC);
	}

	return nullptr;
}

APlayerController* UInventoryHelpers::GetPlayerControllerFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;

	// 1) Instigator controller if it’s a player
	if (AController* InstCtrl = Actor->GetInstigatorController())
	{
		if (APlayerController* PC = Cast<APlayerController>(InstCtrl))
			return PC;
	}

	// 2) Direct controller if a pawn
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (AController* Ctrl = Pawn->GetController())
			if (APlayerController* PC2 = Cast<APlayerController>(Ctrl))
				return PC2;
	}

	// 3) If the owner is a PC
	if (APlayerController* OwnerPC = Cast<APlayerController>(Actor->GetOwner()))
		return OwnerPC;

	// 4) Fallback: local player 0
	return UGameplayStatics::GetPlayerController(Actor, 0);
}

APlayerController* UInventoryHelpers::GetPlayerControllerFromObject(const UObject* Object)
{
	if (!Object) return nullptr;

	// If it's an actor, delegate
	if (const AActor* AsActor = Cast<AActor>(Object))
	{
		return GetPlayerControllerFromActor(const_cast<AActor*>(AsActor));
	}

	// If it's a controller already, just cast it to player controller
	if (const AController* Ctrl = Cast<AController>(Object))
	{
		return Cast<APlayerController>(const_cast<AController*>(Ctrl));
	}

	// If it’s a component, use its owner
	if (const UActorComponent* Comp = Cast<UActorComponent>(Object))
	{
		return GetPlayerControllerFromActor(Comp->GetOwner());
	}

	// Fallback to world player 0 using the world context
	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Object, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		return UGameplayStatics::GetPlayerController(World, 0);
	}

	return nullptr;
}

APlayerController* UInventoryHelpers::GetNetOwningPlayerController(const UObject* WorldContextObject, AActor* Interactor)
{
	if (APlayerController* PC = GetPlayerControllerFromActor(Interactor))
		return PC;

	// Fallback to world 0 if nothing else
	if (WorldContextObject)
	{
		return UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	}
	return nullptr;
}

UItemDataAsset* UInventoryHelpers::FindItemDataByTag(UObject* /*WorldContextObject*/, FGameplayTag ItemIdTag)
{
	if (UInventoryAssetManager* IAM = UInventoryAssetManager::GetOptional())
	{
		return IAM->FindItemDataByTag(ItemIdTag);
	}
	UE_LOG(LogTemp, Warning, TEXT("InventoryAssetManager not configured; FindItemDataByTag fallback returned nullptr."));
	return nullptr;
}

UUserWidget* UInventoryHelpers::CreateWidgetOnInteractor(AActor* Interactor, TSubclassOf<UUserWidget> WidgetClass, bool& bIsLocalPlayer)
{
	bIsLocalPlayer = false;
	if (!Interactor || !*WidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = nullptr;

	// If the interactor is (or owns) a Pawn with a PlayerController, prefer that.
	if (APawn* AsPawn = Cast<APawn>(Interactor))
	{
		PC = Cast<APlayerController>(AsPawn->GetController());
	}
	else
	{
		// Try owner chain -> controller
		if (AController* Ctrl = Cast<AController>(Interactor->GetOwner()))
		{
			PC = Cast<APlayerController>(Ctrl);
		}
	}

	// If still none, try to resolve via instigator
	if (!PC)
	{
		if (APawn* InstPawn = Interactor->GetInstigator())
		{
			PC = Cast<APlayerController>(InstPawn->GetController());
		}
	}

	if (!PC)
	{
		// Fallback: first local player (useful for testing)
		UWorld* World = Interactor->GetWorld();
		if (World && World->GetFirstPlayerController())
		{
			PC = World->GetFirstPlayerController();
		}
	}

	if (!PC)
	{
		return nullptr;
	}

	// Only create the widget if this is the *local* player
	if (!PC->IsLocalController())
	{
		return nullptr;
	}

	bIsLocalPlayer = true;
	return CreateWidget<UUserWidget>(PC, WidgetClass);
}

APlayerController* UInventoryHelpers::ResolvePlayerController(AActor* InActor)
{
	if (!InActor) return nullptr;

	// 1) If it's already a PC
	if (APlayerController* AsPC = Cast<APlayerController>(InActor))
	{
		return AsPC;
	}

	// 2) If it's a pawn, use its controller (if a PC)
	if (APawn* Pawn = Cast<APawn>(InActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			return PC;
		}
	}

	// 3) If it's a controller, try to cast to PC
	if (AController* Ctrl = Cast<AController>(InActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Ctrl))
		{
			return PC;
		}
	}

	// 4) Try the owner chain
	if (AActor* Owner = InActor->GetOwner())
	{
		if (APlayerController* PC = Cast<APlayerController>(Owner))
		{
			return PC;
		}
		if (APawn* OwnerPawn = Cast<APawn>(Owner))
		{
			if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
			{
				return PC;
			}
		}
		if (AController* OwnerCtrl = Cast<AController>(Owner))
		{
			if (APlayerController* PC = Cast<APlayerController>(OwnerCtrl))
			{
				return PC;
			}
		}
	}

	// 5) Fallback: local player 0
	return UGameplayStatics::GetPlayerController(InActor, 0);
}