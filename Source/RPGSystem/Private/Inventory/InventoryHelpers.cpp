#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryAssetManager.h"
#include "Actors/AreaVolumeActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

UInventoryComponent* UInventoryHelpers::GetInventoryComponent(AActor* Actor) { /* (your existing body unchanged) */ }

FString UInventoryHelpers::GetStablePlayerId(AActor* Actor)
{
	if (!Actor) return FString();

	// Prefer PlayerState UniqueId (online subsystem)
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			if (PS->GetUniqueId().IsValid())
			{
				return PS->GetUniqueId()->ToString();
			}
			if (!PS->GetPlayerName().IsEmpty())
			{
				return FString::Printf(TEXT("PSName:%s"), *PS->GetPlayerName());
			}
		}
	}
	// Fallback: stable GUID per save/profile (simple local fallback)
	FGuid Guid = FGuid::NewGuid(); // Replace with your SaveGame/Account GUID if you have one
	return FString::Printf(TEXT("LocalGUID:%s"), *Guid.ToString(EGuidFormats::DigitsWithHyphensInBraces));
}

bool UInventoryHelpers::IsSameParty(AActor* A, AActor* B)
{
	// Stub: you said "everyone is ally / same party for now until wired"
	return true;
}
bool UInventoryHelpers::IsAlly(AActor* A, AActor* B)
{
	// Stub: permissive default
	return true;
}

bool UInventoryHelpers::FindOverlappingAreaTag(AActor* Actor, FGameplayTag& OutAreaTag)
{
	if (!Actor) return false;
	UWorld* W = Actor->GetWorld();
	if (!W) return false;

	TArray<AActor*> Areas;
	UGameplayStatics::GetAllActorsOfClass(W, AAreaVolumeActor::StaticClass(), Areas);

	for (AActor* A : Areas)
	{
		AAreaVolumeActor* AV = Cast<AAreaVolumeActor>(A);
		if (!AV || !AV->Box) continue;

		if (AV->Box->IsOverlappingActor(Actor))
		{
			OutAreaTag = AV->GetAreaTag();
			return OutAreaTag.IsValid();
		}
	}
	return false;
}

// (your existing FindItemDataByTag / ResolveItemPathByTag stay as-is)
