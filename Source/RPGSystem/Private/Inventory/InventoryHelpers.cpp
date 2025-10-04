//All rights Reserved Midnight Entertainment Studios LLC
// Source/RPGSystem/Private/Inventory/InventoryHelpers.cpp

#include "Inventory/InventoryHelpers.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogInvHelpers, Log, All);

// --------------------- Resolve helpers ---------------------

AController* UInventoryHelpers::ResolveController_Internal(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (AController* AsCtrl = Cast<AController>(Actor))
		return AsCtrl;

	if (APawn* AsPawn = Cast<APawn>(Actor))
		return AsPawn->GetController();

	if (const APawn* InstPawn = Cast<APawn>(Actor->GetInstigator()))
		return InstPawn->GetController();

	return nullptr;
}

APlayerState* UInventoryHelpers::ResolvePlayerState_Internal(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (APlayerState* AsPS = Cast<APlayerState>(Actor))
		return AsPS;

	if (const APawn* AsPawn = Cast<APawn>(Actor))
		return AsPawn->GetPlayerState();

	if (AController* C = ResolveController_Internal(Actor))
		return C->PlayerState;

	return nullptr;
}

APlayerController* UInventoryHelpers::ResolvePC_Internal(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (APlayerController* AsPC = Cast<APlayerController>(Actor))
		return AsPC;

	if (AController* C = ResolveController_Internal(Actor))
		return Cast<APlayerController>(C);

	if (const APlayerState* PS = ResolvePlayerState_Internal(Actor))
	{
		if (const APawn* Pawn = PS->GetPawn())
			return Cast<APlayerController>(Pawn->GetController());
	}
	return nullptr;
}

APlayerController* UInventoryHelpers::ResolvePlayerController(AActor* Actor)
{
	return ResolvePC_Internal(Actor);
}

UInventoryComponent* UInventoryHelpers::GetInventoryComponent(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (UInventoryComponent* Direct = Actor->FindComponentByClass<UInventoryComponent>())
		return Direct;

	if (AController* Ctrl = ResolveController_Internal(Actor))
	{
		if (UInventoryComponent* OnCtrl = Ctrl->FindComponentByClass<UInventoryComponent>())
			return OnCtrl;

		if (APawn* Pawn = Ctrl->GetPawn())
		{
			if (UInventoryComponent* OnPawn = Pawn->FindComponentByClass<UInventoryComponent>())
				return OnPawn;
		}
	}

	if (APlayerState* PS = ResolvePlayerState_Internal(Actor))
	{
		if (UInventoryComponent* OnPS = PS->FindComponentByClass<UInventoryComponent>())
			return OnPS;
	}

	return nullptr;
}

// --------------------- UI helpers ---------------------

bool UInventoryHelpers::CreateWidgetOnInteractor(AActor* Interactor, TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& OutWidget)
{
	OutWidget = nullptr;
	if (!Interactor || !*WidgetClass)
		return false;

	if (APlayerController* PC = ResolvePC_Internal(Interactor))
	{
		if (UWorld* World = PC->GetWorld())
		{
			OutWidget = CreateWidget<UUserWidget>(World, WidgetClass);
			if (OutWidget)
			{
				OutWidget->AddToViewport();
				return true;
			}
		}
	}
	return false;
}

// --------------------- Tag → Asset helpers ---------------------

UItemDataAsset* UInventoryHelpers::FindItemDataByTag(UObject* /*WorldContextObject*/, FGameplayTag ItemIDTag, bool bSyncLoad)
{
	if (!ItemIDTag.IsValid())
		return nullptr;

	if (UInventoryAssetManager* AM = UInventoryAssetManager::GetOptional())
	{
		return AM->LoadDataAssetByTag<UItemDataAsset>(ItemIDTag, bSyncLoad);
	}
	return nullptr;
}

// C++-only convenience overload forwards with bSyncLoad=true
UItemDataAsset* UInventoryHelpers::FindItemDataByTag(UObject* WorldContextObject, FGameplayTag ItemIDTag)
{
	return FindItemDataByTag(WorldContextObject, ItemIDTag, /*bSyncLoad=*/true);
}

UDataAsset* UInventoryHelpers::LoadDataAssetByTag(UObject* /*WorldContextObject*/, FGameplayTag Tag, TSubclassOf<UDataAsset> AssetClass, bool bSyncLoad)
{
	if (!AssetClass || !Tag.IsValid())
		return nullptr;

	if (UInventoryAssetManager* AM = UInventoryAssetManager::GetOptional())
	{
		return AM->LoadDataAssetByTag(Tag, AssetClass, bSyncLoad);
	}
	return nullptr;
}

bool UInventoryHelpers::ResolveDataAssetPathByTag(const FGameplayTag& Tag, TSubclassOf<UDataAsset> AssetClass, FSoftObjectPath& OutPath)
{
	OutPath.Reset();
	if (!AssetClass || !Tag.IsValid())
		return false;

	if (const UInventoryAssetManager* AM = UInventoryAssetManager::GetOptional())
	{
		return AM->ResolveDataAssetPathByTag(Tag, AssetClass, OutPath);
	}
	return false;
}

// --------------------- Compatibility stubs ---------------------

bool UInventoryHelpers::ClientRequestTransfer(AActor* Requestor, UInventoryComponent* /*SourceInventory*/, int32 /*SourceIndex*/, UInventoryComponent* /*TargetInventory*/, int32 /*TargetIndex*/)
{
	UE_LOG(LogInvHelpers, Verbose, TEXT("ClientRequestTransfer called (stub). Wire to InventoryComponent server transfer API."));
	return false;
}

bool UInventoryHelpers::TryAddByItemIDTag(UInventoryComponent* /*Inventory*/, FGameplayTag /*ItemIDTag*/, int32 /*Quantity*/, int32& OutAddedIndex)
{
	OutAddedIndex = INDEX_NONE;
	UE_LOG(LogInvHelpers, Verbose, TEXT("TryAddByItemIDTag called (stub). Replace with your InventoryComponent add API."));
	return false;
}
