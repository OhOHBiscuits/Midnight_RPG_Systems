#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDataAsset.generated.h"

/** Optional byproduct definition for fuels (kept from your current system) */
USTRUCT(BlueprintType)
struct FFuelByproduct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
	FGameplayTag ByproductItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fuel")
	int32 Amount = 1;
};

/** Crafting: a station (and optional tier) that can craft this item */
USTRUCT(BlueprintType)
struct FCraftingStationRequirement
{
	GENERATED_BODY()

	/** e.g. Station.Forge, Station.Workbench, Station.Fabricator */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag StationTag;

	/** Optional: require a minimum station tier/quality */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting", meta=(ClampMin="0"))
	int32 MinStationTier = 0;
};

/** Crafting: skill gate (works nicely with GAS Companion attributes/sets) */
USTRUCT(BlueprintType)
struct FSkillRequirement
{
	GENERATED_BODY()

	/** e.g. Skill.Blacksmithing, Skill.Carpentry */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag SkillTag;

	/** Minimum level in that skill to craft */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting", meta=(ClampMin="0"))
	int32 MinLevel = 0;
};

/** Optional unlocks granted to the crafter on first craft (tags make this GAS-friendly) */
USTRUCT(BlueprintType)
struct FCraftingUnlock
{
	GENERATED_BODY()

	/** e.g. Recipe.Sword.Fine, Schematic.Armor.Plates.Iron, Emote.CraftsmanPose */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
	FGameplayTag UnlockTag;
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

	/** (legacy scalar retained) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	float DecayRate = 0.f;

	/** Inventory item result after decay (optional) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftObjectPtr<UItemDataAsset> DecaysInto;

	/** World actor class to spawn after decay (compat with PickupItemActor) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Decay", meta=(EditCondition="bCanDecay"))
	TSoftClassPtr<AActor> DecaysIntoActorClass;

	/** Explicit decay duration parts */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tags")
	TArray<FGameplayTag> AdditionalTags;

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

	// --- Crafting (New) ---
	/** Can this item be crafted at all (via any station/recipe)? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting")
	bool bCraftable = false;

	/** Primary crafting area/skill (e.g. Skill.Blacksmithing). Used for XP + gating. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	FGameplayTag PrimaryCraftSkillTag;

	/** Optional: how hard this is to craft. Use to scale XP and/or failure chances. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0.0"))
	float CraftDifficulty = 1.0f;

	/** Base time to craft (seconds), tweaked by station efficiency, perks, etc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0.0"))
	float CraftTimeSeconds = 3.0f;

	/** Minimum skill level(s) required to craft this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FSkillRequirement> RequiredSkills;

	/** Station(s) that can craft this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FCraftingStationRequirement> RequiredStations;

	/** Optional: recipe identifier(s) that produce this item (if you keep recipes as tags). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FGameplayTag> RecipeTags;

	/** XP the crafter earns for a successful craft (before modifiers). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable", ClampMin="0"))
	int32 BaseCraftXP = 10;

	/** Optional unlocks granted upon first successful craft. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Crafting", meta=(EditCondition="bCraftable"))
	TArray<FCraftingUnlock> FirstCraftUnlocks;

	// --- Heirloom rules (New) ---
	/** If true, this item may be crafted as a one-of-a-kind Heirloom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom")
	bool bHeirloomEligible = false;

	/** When crafted as heirloom, bind the instance to the crafter’s account/ID. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bBindOnCraft = true;

	/** Heirloom instances cannot be traded between players. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bNonTradeableIfHeirloom = true;

	/** Heirloom instances cannot be destroyed/dropped (soft-protect). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bNonDestructibleIfHeirloom = true;

	/** Require a unique custom name when crafting as heirloom. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	bool bUniqueNameRequiredForHeirloom = true;

	/** Optional prefix/suffix shown when naming heirlooms (UX sugar only). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Heirloom", meta=(EditCondition="bHeirloomEligible"))
	FText HeirloomNameHint;

	// --- Crafting metadata for UI (kept from previous pass) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1_Inventory|Crafting")
	bool bCraftingEnabled = false;

	// --- Cook-safe sync helpers ---
	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UStaticMesh* GetWorldMeshSync() const { return WorldMesh.IsNull() ? nullptr : WorldMesh.LoadSynchronous(); }

	UFUNCTION(BlueprintCallable, Category="1_Inventory|ItemData")
	UTexture2D* GetIconSync() const { return Icon.IsNull() ? nullptr : Icon.LoadSynchronous(); }

	// --- Helper queries ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Heirloom")
	bool IsHeirloomEligible() const { return bHeirloomEligible; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crafting")
	bool RequiresAnyStation() const { return bCraftable && RequiredStations.Num() > 0; }
};
