// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataAsset.h"
#include "GameplayTagContainer.h"
#include "WorkstationDataAsset.generated.h"

class UCraftingRecipeDataAsset;
class UUserWidget;

/**
 * Data-driven config for a workstation/world-item.
 * Put this on the ItemDataAsset you use to spawn/tune world actors.
 */
UCLASS(BlueprintType)
class RPGSYSTEM_API UWorkstationDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	/** Recipes this workstation is allowed to run (soft for modding / plug-n-play). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation")
	TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>> AllowedRecipes;

	/** Optional widget to open on interact (soft so mods can swap UI without code). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation|UI")
	TSoftClassPtr<UUserWidget> StationWidgetClass;

	/** Optional tags to mark/type the station. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Workstation|Tags")
	FGameplayTagContainer StationTags;
};