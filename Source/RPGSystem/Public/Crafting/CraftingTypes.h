#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.generated.h"

class UItemDataAsset;
class UCraftingRecipeDataAsset;

/** Where the crafter must be while the craft runs. */
UENUM(BlueprintType)
enum class ECraftPresencePolicy : uint8
{
	CrafterCanLeave UMETA(DisplayName="Crafter Can Leave"),
	CrafterMustRemain UMETA(DisplayName="Crafter Must Remain")
};

/** One input cost (item + quantity). */
USTRUCT(BlueprintType)
struct FCraftItemCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataAsset> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 Quantity = 1;
};

/** One output (item + quantity). */
USTRUCT(BlueprintType)
struct FCraftItemOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDataAsset> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 Quantity = 1;
};

/** Replicated job snapshot for UI & listeners. */
USTRUCT(BlueprintType)
struct FCraftingJob
{
	GENERATED_BODY()

	/** Recipe asset in progress. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCraftingRecipeDataAsset> Recipe = nullptr;

	/** Who started it. */
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Instigator;

	/** Server timestamp range. */
	UPROPERTY(BlueprintReadOnly)
	float StartTime = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float EndTime = 0.f;

	bool IsValid() const { return Recipe != nullptr; }
	float RemainingSeconds(const UWorld* W) const
	{
		const float Now = (W ? W->GetTimeSeconds() : 0.f);
		return FMath::Max(0.f, EndTime - Now);
	}
};
