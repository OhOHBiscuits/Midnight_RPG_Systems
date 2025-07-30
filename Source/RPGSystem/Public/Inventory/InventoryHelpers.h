#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "InventoryHelpers.generated.h"

UCLASS()
class RPGSYSTEM_API UInventoryHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Universal inventory getter. Works for Pawn, PlayerController, PlayerState, or any Actor with an InventoryComponent.
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static UInventoryComponent* GetInventoryComponent(AActor* Actor)
	{
		if (!Actor) return nullptr;

		// 1. Actor itself
		if (UInventoryComponent* Comp = Actor->FindComponentByClass<UInventoryComponent>())
			return Comp;

		// 2. PlayerController
		if (APlayerController* PC = Cast<APlayerController>(Actor))
		{
			if (UInventoryComponent* Comp = PC->FindComponentByClass<UInventoryComponent>())
				return Comp;
			if (PC->PlayerState)
				if (UInventoryComponent* Comp = PC->PlayerState->FindComponentByClass<UInventoryComponent>())
					return Comp;
		}

		// 3. Pawn
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (UInventoryComponent* Comp = Pawn->FindComponentByClass<UInventoryComponent>())
				return Comp;
			if (AController* C = Pawn->GetController())
			{
				if (UInventoryComponent* Comp = C->FindComponentByClass<UInventoryComponent>())
					return Comp;
				if (C->PlayerState)
					if (UInventoryComponent* Comp = C->PlayerState->FindComponentByClass<UInventoryComponent>())
						return Comp;
			}
		}

		// 4. Owner (traverse chain)
		if (AActor* Owner = Actor->GetOwner())
			return GetInventoryComponent(Owner);

		return nullptr;
	}
};
