// ItemDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.generated.h"

class AActor;

UCLASS(BlueprintType)
class RPGSYSTEM_API UItemDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UItemDataAsset();

    /** Display name for UI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Display")
    FText ItemName;

    /** Full description shown on inspect */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Display", meta=(MultiLine=true))
    FText Description;

    /** Short description for tooltips */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Display", meta=(MultiLine=true))
    FText ShortDescription;

    /** Icon displayed in inventory */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Visual")
    TSoftObjectPtr<UTexture2D> ItemIcon;

    /** Mesh to spawn when dropped */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Visual")
    TSoftObjectPtr<UStaticMesh> ItemMesh;

    /** Class to spawn when world-dropped */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|World")
    TSoftClassPtr<AActor> DropActorClass;

    /** Gameplay tags assigned in editor */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTag ItemIDTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTag CategoryTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTag SubTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTag TypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTag RarityTag;

    /** Weight in inventory calculations */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Stats")
    float Weight = 1.f;

    /** Volume in inventory calculations */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Stats")
    float Volume = 1.f;

    /** Maximum stack size */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Stats")
    int32 MaxStackSize = 1;

    /** Decay properties */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Decay")
    bool bCanDecay = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Decay", meta=(EditCondition="bCanDecay"))
    float DecayRate = 0.f;

    /** Item to spawn into when decay completes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item|Decay", meta=(EditCondition="bCanDecay"))
    TSoftObjectPtr<UItemDataAsset> DecayInto;
};