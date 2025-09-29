#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.generated.h"

// Forward decls
class AActor;
class UStaticMesh;
class UTexture2D;
class UAnimMontage;
class UUserWidget;
class UGameplayEffect;

/** Optional byproduct definition for fuels (kept from your version) */
USTRUCT(BlueprintType)
struct FFuelByproduct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
	FGameplayTag ByproductItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
	int32 Amount = 1;
};

USTRUCT(BlueprintType)
struct FCraftingStationRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag StationTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting", meta=(ClampMin="0"))
	int32 MinStationTier = 0;
};

USTRUCT(BlueprintType)
struct FSkillRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag SkillTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting", meta=(ClampMin="0"))
	int32 MinLevel = 0;
};

USTRUCT(BlueprintType)
struct FCraftingUnlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag UnlockTag;
};

/** NEW: richer UI/logic action definition (replaces bare AllowedActions) */
USTRUCT(BlueprintType)
struct FItemAction
{
	GENERATED_BODY()

	/** Tag you’ll key from code, e.g. Action.Use, Action.Equip */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	FGameplayTag ActionTag;

	/** Player-facing button name (“Eat”, “Equip”, …) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	FText DisplayName;

	/** Optional montage to play for UX */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	TSoftObjectPtr<UAnimMontage> Montage;

	/** If true, you intend to apply effects directly when this action triggers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action")
	bool bApplyEffectsDirectly = false;

	/** Your own tag→effect mapping */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action", meta=(EditCondition="bApplyEffectsDirectly"))
	TArray<FGameplayTag> EffectTags;

	/** Optional GAS effects (soft class). Only add GameplayAbilities module if you actually use these. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Action", meta=(EditCondition="bApplyEffectsDirectly"))
	TArray<TSoftClassPtr<UGameplayEffect>> GameplayEffects;
};

/**
 * Base item data (parent of everything).
 */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI", meta=(MultiLine="true"))
	FText Description;

	// --- Physical ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(ClampMin="0.0"))
	float Volume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	bool bStackable = true;

	// --- Decay ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay")
	bool bCanDecay = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayRate = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftObjectPtr<UItemDataAsset> DecaysInto;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftClassPtr<AActor> DecaysIntoActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay", ClampMin="0.0"))
	float DecayDays = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay", ClampMin="0.0"))
	float DecayHours = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay", ClampMin="0.0"))
	float DecayMinutes = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay", ClampMin="0.0"))
	float DecaySeconds = 0.0f;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Decay")
	float GetTotalDecaySeconds() const
	{
		return DecayDays * 86400.0f + DecayHours * 3600.0f + DecayMinutes * 60.0f + DecaySeconds;
	}

	// --- Durability ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability")
	bool bHasDurability = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Durability", meta=(EditCondition="bHasDurability", ClampMin="1"))
	int32 MaxDurability = 100;

	// --- Fuel ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel")
	bool bIsFuel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(EditCondition="bIsFuel"))
	float BurnRate = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fuel", meta=(EditCondition="bIsFuel"))
	TArray<FFuelByproduct> FuelByproducts;

	// --- World ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftClassPtr<AActor> WorldActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	// --- Tags (legacy) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemCategory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag ItemSubCategory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	FGameplayTag Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	TArray<FGameplayTag> AdditionalTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Equipment")
	TArray<FGameplayTag> PreferredEquipSlots;

	/** Legacy, kept for compatibility */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	TArray<FGameplayTag> AllowedActions;

	// --- NEW: Rich action definitions ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Actions")
	TArray<FItemAction> ActionDefinitions;

	// --- Misc ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ItemData")
	float EfficiencyRating = 1.0f;

	// --- Fuel duration helper ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel", meta=(EditCondition="bIsFuel", ClampMin="0.0"))
	float BurnDays = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel", meta=(EditCondition="bIsFuel", ClampMin="0.0"))
	float BurnHours = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel", meta=(EditCondition="bIsFuel", ClampMin="0.0"))
	float BurnMinutes = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel", meta=(EditCondition="bIsFuel", ClampMin="0.0"))
	float BurnSeconds = 30.0f;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	float GetTotalBurnSeconds() const
	{
		return BurnDays * 86400.0f + BurnHours * 3600.0f + BurnMinutes * 60.0f + BurnSeconds;
	}

	// --- Crafting (kept) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting")
	bool bCraftable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	FGameplayTag PrimaryCraftSkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0.0"))
	float CraftDifficulty = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0.0"))
	float CraftTimeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FSkillRequirement> RequiredSkills;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FCraftingStationRequirement> RequiredStations;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FGameplayTag> RecipeTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0"))
	int32 BaseCraftXP = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FCraftingUnlock> FirstCraftUnlocks;

	// --- Heirloom rules (kept) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom")
	bool bHeirloomEligible = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bBindOnCraft = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bNonTradeableIfHeirloom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bNonDestructibleIfHeirloom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bUniqueNameRequiredForHeirloom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	FText HeirloomNameHint;

	// --- Crafting metadata for UI (kept) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting")
	bool bCraftingEnabled = false;

	// --- Cook-safe sync helpers (kept) ---
	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UStaticMesh* GetWorldMeshSync() const { return WorldMesh.IsNull() ? nullptr : WorldMesh.LoadSynchronous(); }

	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UTexture2D* GetIconSync() const { return Icon.IsNull() ? nullptr : Icon.LoadSynchronous(); }

	// --- Queries (kept) ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Heirloom")
	bool IsHeirloomEligible() const { return bHeirloomEligible; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crafting")
	bool RequiresAnyStation() const { return bCraftable && RequiredStations.Num() > 0; }

	// --- NEW: Actions API ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Actions")
	bool HasAction(const FGameplayTag& ActionTag) const;

	/** Returns found action or a static empty action; bFound says if it matched. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Actions")
	const FItemAction& GetActionOrDefault(const FGameplayTag& ActionTag, bool& bFound) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Actions")
	void GetActionTags(TArray<FGameplayTag>& OutTags) const;

protected:
	const FItemAction* FindAction(const FGameplayTag& ActionTag) const;
};
