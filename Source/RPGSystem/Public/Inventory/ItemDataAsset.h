#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FFuelByproduct
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
    FGameplayTag ByproductItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
    int32 Amount = 1;
};

UCLASS(BlueprintType)
class RPGSYSTEM_API UItemDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:

    // ---------- UI / General ----------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FGameplayTag ItemIDTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    FText Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    FText ShortDescription;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    float Weight = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    float Volume = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    int32 MaxStackSize = 1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bStackable = true;

    // ---------- Decay ----------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay")
    bool bCanDecay = false;

    // Legacy single-value seconds (kept for older assets)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
    float DecayRate = 0.f;

    // NEW: Time breakdown like Fuel
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay", meta=(EditCondition="bCanDecay"))
    float DecayDays = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay", meta=(EditCondition="bCanDecay"))
    float DecayHours = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay", meta=(EditCondition="bCanDecay"))
    float DecayMinutes = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Decay", meta=(EditCondition="bCanDecay"))
    float DecaySeconds = 0.0f;

    // Inventory output when this item decays (used by containers/inventories)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
    TSoftObjectPtr<UItemDataAsset> DecaysInto;

    // Optional: world actor to spawn when a *world pickup* finishes decaying
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
    TSoftClassPtr<AActor> DecaysIntoActorClass;

    // Helper: total seconds for decay. If all new fields are 0, falls back to DecayRate.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Decay")
    float GetTotalDecaySeconds() const
    {
        const float Total =
            DecayDays    * 86400.0f +
            DecayHours   * 3600.0f  +
            DecayMinutes * 60.0f    +
            DecaySeconds;

        return (Total > 0.0f) ? Total : FMath::Max(0.0f, DecayRate);
    }

    // ---------- Durability ----------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability")
    bool bHasDurability = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability", meta=(EditCondition="bHasDurability"))
    int32 MaxDurability = 100;

    // ---------- Fuel ----------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel")
    bool bIsFuel = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(EditCondition="bIsFuel"))
    float BurnRate = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel")
    TArray<FFuelByproduct> FuelByproducts;

    // World Placement
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
    TSoftClassPtr<AActor> WorldActorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    // Tags
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
    FGameplayTag ItemType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
    FGameplayTag ItemCategory;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
    FGameplayTag ItemSubCategory;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
    FGameplayTag Rarity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
    TArray<FGameplayTag> AllowedActions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemData")
    float EfficiencyRating = 1.0f;

    // ---------- Fuel time breakdown ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
    float BurnDays = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
    float BurnHours = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
    float BurnMinutes = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
    float BurnSeconds = 30.0f; // Default

    // Helper (C++ only)
    float GetTotalBurnSeconds() const
    {
        return BurnDays * 86400.0f + BurnHours * 3600.0f + BurnMinutes * 60.0f + BurnSeconds;
    }
};
