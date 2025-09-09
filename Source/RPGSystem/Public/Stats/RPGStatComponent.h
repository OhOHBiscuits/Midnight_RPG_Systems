#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StatProviderInterface.h"
#include "RPGStatComponent.generated.h"

class UStatSetDataAsset;
class URPGStatComponent;

// ---------- Scalars ----------

USTRUCT()
struct FRPGScalarEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	FText Name;

	UPROPERTY()
	float Value = 0.f;
};

USTRUCT()
struct FRPGScalarList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGScalarEntry> Items;

	// Not replicated; set by owner to allow callbacks to rebuild caches
	UPROPERTY(Transient)
	URPGStatComponent* Owner = nullptr;

	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGScalarEntry, FRPGScalarList>(Items, DeltaParams, *this);
	}

	// declared here, defined in .cpp to avoid “use of undefined type” errors
	void PostReplicatedAdd   (const TArrayView<int32>& AddedIndices,   int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
};

template<> struct TStructOpsTypeTraits<FRPGScalarList> : public TStructOpsTypeTraitsBase2<FRPGScalarList>
{
	enum { WithNetDeltaSerializer = true };
};

// ---------- Vitals (Current / Max) ----------

USTRUCT()
struct FRPGVitalEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	FText Name;

	UPROPERTY()
	float Current = 0.f;

	UPROPERTY()
	float Max = 100.f;
};

USTRUCT()
struct FRPGVitalList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGVitalEntry> Items;

	UPROPERTY(Transient)
	URPGStatComponent* Owner = nullptr;

	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGVitalEntry, FRPGVitalList>(Items, DeltaParams, *this);
	}

	void PostReplicatedAdd   (const TArrayView<int32>& AddedIndices,   int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
};

template<> struct TStructOpsTypeTraits<FRPGVitalList> : public TStructOpsTypeTraitsBase2<FRPGVitalList>
{
	enum { WithNetDeltaSerializer = true };
};

// ---------- Skills (Level / XP / XPToNext) ----------

USTRUCT()
struct FRPGSkillEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	FText Name;

	UPROPERTY()
	int32 Level = 1;

	UPROPERTY()
	float XP = 0.f;

	UPROPERTY()
	float XPToNext = 100.f;
};

USTRUCT()
struct FRPGSkillList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGSkillEntry> Items;

	UPROPERTY(Transient)
	URPGStatComponent* Owner = nullptr;

	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGSkillEntry, FRPGSkillList>(Items, DeltaParams, *this);
	}

	void PostReplicatedAdd   (const TArrayView<int32>& AddedIndices,   int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PostReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
};

template<> struct TStructOpsTypeTraits<FRPGSkillList> : public TStructOpsTypeTraitsBase2<FRPGSkillList>
{
	enum { WithNetDeltaSerializer = true };
};

// ---------- Delegates for UI ----------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScalarChanged, FGameplayTag, StatTag, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVitalChanged,  FGameplayTag, StatTag, float, NewCurrent, float, NewMax);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillChanged,  FGameplayTag, StatTag, int32, NewLevel, float, NewXP);

// ---------- Component ----------

UCLASS(BlueprintType, Blueprintable, ClassGroup=(RPG), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGStatComponent : public UActorComponent, public IStatProviderInterface
{
	GENERATED_BODY()

public:
	URPGStatComponent();

	// Data-driven seed: assign one or more sets in BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Setup")
	TArray<TObjectPtr<UStatSetDataAsset>> StatSets;

	// Replicated live state
	UPROPERTY(Replicated)
	FRPGScalarList ScalarStats;

	UPROPERTY(Replicated)
	FRPGVitalList  Vitals;

	UPROPERTY(Replicated)
	FRPGSkillList  Skills;

	// UI events (client)
	UPROPERTY(BlueprintAssignable, Category="Stats|Events")
	FOnScalarChanged OnScalarChanged;

	UPROPERTY(BlueprintAssignable, Category="Stats|Events")
	FOnVitalChanged OnVitalChanged;

	UPROPERTY(BlueprintAssignable, Category="Stats|Events")
	FOnSkillChanged OnSkillChanged;

	// ---- Interface (implementations in .cpp) ----
	virtual float GetStat_Implementation(FGameplayTag Tag) const override;
	virtual void  SetStat_Implementation(FGameplayTag Tag, float NewValue, bool bClampToValidRange) override;
	virtual void  AddToStat_Implementation(FGameplayTag Tag, float Delta, bool bClampToValidRange) override;

	// Seeding from assets (server only)
	UFUNCTION(BlueprintCallable, Category="Stats|Setup")
	void InitializeFromStatSets(bool bClearExisting = false);

	// Utilities
	UFUNCTION(BlueprintCallable, Category="Stats")
	bool HasStat(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="Stats")
	void GetAllStatTags(TArray<FGameplayTag>& OutTags) const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend struct FRPGScalarList;
	friend struct FRPGVitalList;
	friend struct FRPGSkillList;
	void RebuildCaches();
	// quick lookups
	TMap<FGameplayTag, int32> ScalarIndex;
	TMap<FGameplayTag, int32> VitalIndex;
	TMap<FGameplayTag, int32> SkillIndex;

	bool bInitializedFromSets = false;

	// cache mgmt
	void RebuildCaches();

	// find or add helpers (server)
	int32 FindScalarIdx(FGameplayTag Tag) const;
	int32 FindVitalIdx (FGameplayTag Tag) const;
	int32 FindSkillIdx (FGameplayTag Tag) const;

	int32 FindOrAddScalar(FGameplayTag Tag, const FText* DisplayName = nullptr);
	int32 FindOrAddVital (FGameplayTag Tag, const FText* DisplayName = nullptr, float InitCurrent = 0.f, float InitMax = 100.f);
	int32 FindOrAddSkill (FGameplayTag Tag, const FText* DisplayName = nullptr, int32 InitLevel = 1);

	// skill rule
	float ComputeXPToNext(int32 Level) const;

	// client UI notifies
	void BroadcastScalar(const FRPGScalarEntry& E) const;
	void BroadcastVital (const FRPGVitalEntry&  E) const;
	void BroadcastSkill (const FRPGSkillEntry&  E) const;

	// RPCs for client requests (optional; safe-guarded)
	UFUNCTION(Server, Reliable)
	void ServerSetStat(FGameplayTag Tag, float NewValue, bool bClamp);
	UFUNCTION(Server, Reliable)
	void ServerAddToStat(FGameplayTag Tag, float Delta, bool bClamp);
};
