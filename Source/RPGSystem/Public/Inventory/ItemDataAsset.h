#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.generated.h"

class UCraftingRecipeDataAsset;

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
	// --- UI ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSoftObjectPtr<UTexture2D> Icon;

	// Searchable in cooked
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(AssetRegistrySearchable))
	FGameplayTag ItemIDTag;

	// --- Display ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	FText ShortDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	FText Description;

	// --- Physical ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	bool bStackable = true;

	// --- Decay ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay")
	bool bCanDecay = false;

	/** Rate scalar for your own systems (kept for backward compat) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayRate = 0.f;

	/** Optional: the item this decays into (inventory item result) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftObjectPtr<UItemDataAsset> DecaysInto;

	/** NEW: optional actor class to spawn in the world after decay (compat for PickupItemActor) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftClassPtr<AActor> DecaysIntoActorClass;

	/** NEW: explicit decay duration, split into components, summed by GetTotalDecaySeconds() */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayDays = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayHours = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayMinutes = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecaySeconds = 0.0f;

	/** NEW: compat function expected by DecayComponent/PickupItemActor */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Decay")
	float GetTotalDecaySeconds() const
	{
		return DecayDays * 86400.0f + DecayHours * 3600.0f + DecayMinutes * 60.0f + DecaySeconds;
	}

	// --- Durability ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability")
	bool bHasDurability = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability", meta=(EditCondition="bHasDurability"))
	int32 MaxDurability = 100;

	// --- Fuel ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel")
	bool bIsFuel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(EditCondition="bIsFuel"))
	float BurnRate = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel")
	TArray<FFuelByproduct> FuelByproducts;

	// --- World ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftClassPtr<AActor> WorldActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	// --- Tags ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemCategory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemSubCategory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	TArray<FGameplayTag> AllowedActions;

	// --- Misc ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemData")
	float EfficiencyRating = 1.0f;

	// --- Fuel duration (helper) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnDays = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnHours = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnMinutes = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnSeconds = 30.0f;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	float GetTotalBurnSeconds() const
	{
		return BurnDays * 86400.0f + BurnHours * 3600.0f + BurnMinutes * 60.0f + BurnSeconds;
	}

	// --- Crafting metadata for UI (kept from previous pass) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting")
	bool bCraftingEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting", meta=(EditCondition="bCraftingEnabled"))
	TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>> CraftableRecipes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting", meta=(EditCondition="bCraftingEnabled"))
	TArray<TSoftObjectPtr<UCraftingRecipeDataAsset>> UsedInRecipes;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory|Crafting")
	bool CanBeCrafted() const { return bCraftingEnabled && CraftableRecipes.Num() > 0; }

	// --- Cook-safe sync helpers ---
	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UStaticMesh* GetWorldMeshSync() const { return WorldMesh.IsNull() ? nullptr : WorldMesh.LoadSynchronous(); }

	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UTexture2D* GetIconSync() const { return Icon.IsNull() ? nullptr : Icon.LoadSynchronous(); }
};
