#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StatSetDataAsset.h"
#include "StatProviderInterface.h"
#include "RPGStatComponent.generated.h"

// Forward so our helper can take a pointer without needing the full type yet.
class URPGStatComponent;

/** Free helper used by FastArray PostReplicated* callbacks (defined in .cpp). */
RPGSYSTEM_API void RPGStat_NetPostReplicated(URPGStatComponent* Owner);

// ---------- UI Events ----------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScalarChanged, FGameplayTag, Tag, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVitalChanged,  FGameplayTag, Tag, float, NewCurrent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillChanged, FGameplayTag, Tag, int32, NewLevel, float, NewXP);

// ---------- FastArray Items ----------
USTRUCT()
struct RPGSYSTEM_API FRPGScalarEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY() FGameplayTag Tag;
	UPROPERTY() float        Value = 0.f;
};

USTRUCT()
struct RPGSYSTEM_API FRPGVitalEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY() FGameplayTag Tag;
	UPROPERTY() float        Current = 0.f;
	UPROPERTY() float        Max     = 100.f;
};

USTRUCT()
struct RPGSYSTEM_API FRPGSkillEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY() FGameplayTag Tag;
	UPROPERTY() int32        Level     = 0;
	UPROPERTY() float        XP        = 0.f;
	UPROPERTY() float        XPToNext  = 100.f;
};

// ---------- FastArray Lists ----------
USTRUCT()
struct RPGSYSTEM_API FRPGScalarList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY() TArray<FRPGScalarEntry> Items;

	URPGStatComponent* Owner = nullptr;
	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGScalarEntry, FRPGScalarList>(Items, DeltaParms, *this);
	}

	// Use helper so we don't need the full component type here.
	void PostReplicatedAdd   (const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedChange(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedRemove(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
};
template<> struct TStructOpsTypeTraits<FRPGScalarList> : public TStructOpsTypeTraitsBase2<FRPGScalarList> { enum { WithNetDeltaSerializer = true, WithNetSharedSerialization = true }; };

USTRUCT()
struct RPGSYSTEM_API FRPGVitalList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY() TArray<FRPGVitalEntry> Items;

	URPGStatComponent* Owner = nullptr;
	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGVitalEntry, FRPGVitalList>(Items, DeltaParms, *this);
	}

	void PostReplicatedAdd   (const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedChange(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedRemove(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
};
template<> struct TStructOpsTypeTraits<FRPGVitalList> : public TStructOpsTypeTraitsBase2<FRPGVitalList> { enum { WithNetDeltaSerializer = true, WithNetSharedSerialization = true }; };

USTRUCT()
struct RPGSYSTEM_API FRPGSkillList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY() TArray<FRPGSkillEntry> Items;

	URPGStatComponent* Owner = nullptr;
	void Register(URPGStatComponent* InOwner) { Owner = InOwner; }

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRPGSkillEntry, FRPGSkillList>(Items, DeltaParms, *this);
	}

	void PostReplicatedAdd   (const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedChange(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
	void PostReplicatedRemove(const TArrayView<int32>&, int32) { RPGStat_NetPostReplicated(Owner); }
};
template<> struct TStructOpsTypeTraits<FRPGSkillList> : public TStructOpsTypeTraitsBase2<FRPGSkillList> { enum { WithNetDeltaSerializer = true, WithNetSharedSerialization = true }; };

// ---------- Component ----------
UCLASS(BlueprintType, Blueprintable, ClassGroup=(RPG), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGStatComponent : public UActorComponent, public IStatProviderInterface
{
	GENERATED_BODY()

public:
	URPGStatComponent();

	// ---- Setup ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Setup")
	TArray<TObjectPtr<UStatSetDataAsset>> InitialStatSets;

	/** Rebuilds caches & broadcasts changes; callable by FastArray helpers. */
	void RebuildCaches();

	/** Re/initializes from InitialStatSets. bClearExisting: wipe before apply. */
	UFUNCTION(BlueprintCallable, Category="Stats|Setup")
	void InitializeFromStatSets(bool bClearExisting);


	// ---- IStatProviderInterface (BlueprintNativeEvent) ----
	virtual float GetStat_Implementation(FGameplayTag Tag, float DefaultValue) const override;
	virtual void  SetStat_Implementation(FGameplayTag Tag, float NewValue)      override;
	virtual void  AddToStat_Implementation(FGameplayTag Tag, float Delta)       override;

	// ---- Replication ----
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---- Events for UI ----
	UPROPERTY(BlueprintAssignable) FOnScalarChanged OnScalarChanged;
	UPROPERTY(BlueprintAssignable) FOnVitalChanged  OnVitalChanged;
	UPROPERTY(BlueprintAssignable) FOnSkillChanged  OnSkillChanged;

protected:
	/** If true (default), the component will automatically call InitializeFromStatSets() at BeginPlay when it has valid StatSets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Setup")
	bool bAutoInitializeFromSets = true;

	/** If Auto Initialize is enabled, pass this flag to InitializeFromStatSets(). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Setup", meta=(EditCondition="bAutoInitializeFromSets"))
	bool bClearExistingOnAutoInit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats|Setup")
	TArray<TObjectPtr<UStatSetDataAsset>> StatSets;
	
	virtual void BeginPlay() override;
	// ---- Replicated data ----
	UPROPERTY(Replicated) FRPGScalarList ScalarStats;
	UPROPERTY(Replicated) FRPGVitalList  Vitals;
	UPROPERTY(Replicated) FRPGSkillList  Skills;

	// ---- Tag -> index caches ----
	UPROPERTY(Transient) TMap<FGameplayTag, int32> ScalarIndex;
	UPROPERTY(Transient) TMap<FGameplayTag, int32> VitalIndex;
	UPROPERTY(Transient) TMap<FGameplayTag, int32> SkillIndex;

	// ---- Helpers ----
	int32 FindScalarIdx(FGameplayTag Tag) const { if (const int32* F = ScalarIndex.Find(Tag)) return *F; return INDEX_NONE; }
	int32 FindVitalIdx (FGameplayTag Tag) const { if (const int32* F = VitalIndex .Find(Tag)) return *F; return INDEX_NONE; }
	int32 FindSkillIdx (FGameplayTag Tag) const { if (const int32* F = SkillIndex .Find(Tag)) return *F; return INDEX_NONE; }

	void BroadcastScalar(const FRPGScalarEntry& E) const { OnScalarChanged.Broadcast(E.Tag, E.Value); }
	void BroadcastVital (const FRPGVitalEntry&  E) const { OnVitalChanged .Broadcast(E.Tag, E.Current); }
	void BroadcastSkill (const FRPGSkillEntry&  E) const { OnSkillChanged .Broadcast(E.Tag, E.Level, E.XP); }
};
