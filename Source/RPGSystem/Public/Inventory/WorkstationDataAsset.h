// WorkstationDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "ItemDataAsset.h"            // your base item DA
#include "WorkstationDataAsset.generated.h"

class UCraftingRecipeDataAsset;

UCLASS(BlueprintType)
class RPGSYSTEM_API UWorkstationDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	/** Soft references allow mods to plug in recipes without hard-coupling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation")
	TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>> AllowedRecipes;

	UFUNCTION(BlueprintCallable, Category="Workstation")
	bool IsRecipeAllowed(const UCraftingRecipeDataAsset* Recipe) const;
};
