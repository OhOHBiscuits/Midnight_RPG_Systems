#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StatSetDataAsset.generated.h"

UCLASS(BlueprintType)
class RPGSYSTEM_API UStatSetDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// Default values for any stat tag. Mods can add their own tags here.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	TMap<FGameplayTag, float> Defaults;
};
