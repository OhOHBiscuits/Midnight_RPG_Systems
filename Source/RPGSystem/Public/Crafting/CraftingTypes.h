// CraftingTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CraftingTypes.generated.h"

class UCraftingRecipeDataAsset;
class AActor;

/** How present the crafter must be for a job. */
UENUM(BlueprintType)
enum class ECraftPresencePolicy : uint8
{
	CrafterMustRemain   UMETA(DisplayName="Crafter Must Remain"),
	CrafterCanLeave     UMETA(DisplayName="Crafter Can Leave"),
	CraftingIsRemote    UMETA(DisplayName="Remote / Automated"),
};

/** Simple item + quantity pair by tag id (adjust to your item system). */
USTRUCT(BlueprintType)
struct FCraftingItemQuantity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;
};
USTRUCT(BlueprintType)
struct FCraftItemCost : public FCraftingItemQuantity
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FCraftItemOutput : public FCraftingItemQuantity
{
	GENERATED_BODY()
};
/** One in-flight crafting job. */
USTRUCT(BlueprintType)
struct FCraftingJob
{
	GENERATED_BODY()

	/** Recipe being crafted. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCraftingRecipeDataAsset> Recipe = nullptr;

	/** Who started it (not replicated as a hard pointer). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Instigator;

	/** How many times to craft this recipe in one go. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Count = 1;

	/** Server time when job started. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StartTime = 0.f;

	/** Server time when job should complete. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EndTime   = 0.f;
};
