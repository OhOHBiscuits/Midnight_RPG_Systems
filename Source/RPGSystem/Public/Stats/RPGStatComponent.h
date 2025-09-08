// RPGStatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RPGStatComponent.generated.h"

class URPGStatComponent;

/** One replicated stat row */
USTRUCT(BlueprintType)
struct FRPGStatEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.f;
};

/** FastArray wrapper for all stats on the component */
USTRUCT()
struct FRPGStatList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGStatEntry> Items;

	/** Back-pointer so FastArray callbacks can rebuild the component’s cache */
	UPROPERTY(NotReplicated, Transient)
	URPGStatComponent* Owner = nullptr;

	/** FastArray plumb-in */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

	/** FastArray callbacks (defined in .cpp so we can see URPGStatComponent) */
	void PostReplicatedAdd   (const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedRemove(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits<FRPGStatList> : public TStructOpsTypeTraitsBase2<FRPGStatList>
{
	enum { WithNetDeltaSerializer = true };
};

/** Fired whenever a stat changes (server or after client receives replication) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, FGameplayTag, Tag, float, NewValue);

/**
 * Lightweight, data-driven stat store (GAS-friendly).
 * Replicates via FastArray, keeps a local map cache for fast lookups.
 */
UCLASS(ClassGroup=(RPG), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URPGStatComponent();

	// --- UActorComponent
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRegister() override;

	// --- Query API

	/** Get a stat. If not found, returns DefaultValue (0 by default). */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	float GetStat(FGameplayTag Tag, float DefaultValue = 0.f) const;

	/** Returns true if a stat row exists. */
	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	bool HasStat(FGameplayTag Tag) const { return FindIndex(Tag) != INDEX_NONE; }

	// --- Mutate API (call on server, or will route via RPC) ---

	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	void SetStat(FGameplayTag Tag, float NewValue);

	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	void AddToStat(FGameplayTag Tag, float Delta);

	UFUNCTION(BlueprintCallable, Category="RPG|Stats")
	void RemoveFromStat(FGameplayTag Tag, float Delta) { AddToStat(Tag, -Delta); }

	/** UI can bind to this for immediate updates on clients */
	UPROPERTY(BlueprintAssignable, Category="RPG|Stats")
	FOnStatChanged OnStatChanged;

	/** Rebuild the local cache from replicated array (called by FastArray hooks) */
	void RebuildCache();

protected:
	/** The replicated stat list */
	UPROPERTY(Replicated)
	FRPGStatList ReplicatedStats;

	/** Client-side cache for quick lookups */
	TMap<FGameplayTag, float> Cache;

	// --- Internals ---
	int32 FindIndex(FGameplayTag Tag) const;
	void SetStat_Internal(FGameplayTag Tag, float NewValue);

	// --- RPCs (so BP/users can safely call from client) ---
	UFUNCTION(Server, Reliable)
	void ServerSetStat(FGameplayTag Tag, float NewValue);

	UFUNCTION(Server, Reliable)
	void ServerAddToStat(FGameplayTag Tag, float Delta);
};
