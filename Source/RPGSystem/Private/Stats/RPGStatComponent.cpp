#include "Stats/RPGStatComponent.h"
#include "Stats/StatSetDataAsset.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// ===== FRPGScalarList callbacks =====
void FRPGScalarList::PostReplicatedAdd(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGScalarList::PostReplicatedChange(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGScalarList::PostReplicatedRemove(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}

// ===== FRPGVitalList callbacks =====
void FRPGVitalList::PostReplicatedAdd(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGVitalList::PostReplicatedChange(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGVitalList::PostReplicatedRemove(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}

// ===== FRPGSkillList callbacks =====
void FRPGSkillList::PostReplicatedAdd(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGSkillList::PostReplicatedChange(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}
void FRPGSkillList::PostReplicatedRemove(const TArrayView<int32>&, int32)
{
	if (Owner) Owner->RebuildCaches();
}

// ===== Component =====

URPGStatComponent::URPGStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	ScalarStats.Register(this);
	Vitals.Register(this);
	Skills.Register(this);
}

void URPGStatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && !bInitializedFromSets)
	{
		InitializeFromStatSets(false);
	}
}

void URPGStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
	Super::GetLifetimeReplicatedProps(Out);

	DOREPLIFETIME(URPGStatComponent, ScalarStats);
	DOREPLIFETIME(URPGStatComponent, Vitals);
	DOREPLIFETIME(URPGStatComponent, Skills);
}

// -------- caches --------

void URPGStatComponent::RebuildCaches()
{
	ScalarIndex.Reset();
	for (int32 i = 0; i < ScalarStats.Items.Num(); ++i)
	{
		ScalarIndex.Add(ScalarStats.Items[i].Tag, i);
		BroadcastScalar(ScalarStats.Items[i]); // one-shot UI refresh if needed
	}
	VitalIndex.Reset();
	for (int32 i = 0; i < Vitals.Items.Num(); ++i)
	{
		VitalIndex.Add(Vitals.Items[i].Tag, i);
		BroadcastVital(Vitals.Items[i]);
	}
	SkillIndex.Reset();
	for (int32 i = 0; i < Skills.Items.Num(); ++i)
	{
		SkillIndex.Add(Skills.Items[i].Tag, i);
		BroadcastSkill(Skills.Items[i]);
	}
}

int32 URPGStatComponent::FindScalarIdx(FGameplayTag Tag) const { if (const int32* p = ScalarIndex.Find(Tag)) return *p; return INDEX_NONE; }
int32 URPGStatComponent::FindVitalIdx (FGameplayTag Tag) const { if (const int32* p = VitalIndex .Find(Tag)) return *p; return INDEX_NONE; }
int32 URPGStatComponent::FindSkillIdx (FGameplayTag Tag) const { if (const int32* p = SkillIndex .Find(Tag)) return *p; return INDEX_NONE; }

int32 URPGStatComponent::FindOrAddScalar(FGameplayTag Tag, const FText* DisplayName)
{
	int32 Idx = FindScalarIdx(Tag);
	if (Idx != INDEX_NONE) return Idx;

	FRPGScalarEntry& E = ScalarStats.Items.AddDefaulted_GetRef();
	E.Tag  = Tag;
	E.Name = DisplayName ? *DisplayName : FText::GetEmpty();

	Idx = ScalarStats.Items.Num() - 1;
	ScalarIndex.Add(Tag, Idx);
	ScalarStats.MarkItemDirty(E);
	return Idx;
}

int32 URPGStatComponent::FindOrAddVital(FGameplayTag Tag, const FText* DisplayName, float InitCurrent, float InitMax)
{
	int32 Idx = FindVitalIdx(Tag);
	if (Idx != INDEX_NONE) return Idx;

	FRPGVitalEntry& E = Vitals.Items.AddDefaulted_GetRef();
	E.Tag     = Tag;
	E.Name    = DisplayName ? *DisplayName : FText::GetEmpty();
	E.Max     = FMath::Max(0.f, InitMax);
	E.Current = FMath::Clamp(InitCurrent, 0.f, E.Max);

	Idx = Vitals.Items.Num() - 1;
	VitalIndex.Add(Tag, Idx);
	Vitals.MarkItemDirty(E);
	return Idx;
}

int32 URPGStatComponent::FindOrAddSkill(FGameplayTag Tag, const FText* DisplayName, int32 InitLevel)
{
	int32 Idx = FindSkillIdx(Tag);
	if (Idx != INDEX_NONE) return Idx;

	FRPGSkillEntry& E = Skills.Items.AddDefaulted_GetRef();
	E.Tag     = Tag;
	E.Name    = DisplayName ? *DisplayName : FText::GetEmpty();
	E.Level   = FMath::Max(1, InitLevel);
	E.XP      = 0.f;
	E.XPToNext = ComputeXPToNext(E.Level);

	Idx = Skills.Items.Num() - 1;
	SkillIndex.Add(Tag, Idx);
	Skills.MarkItemDirty(E);
	return Idx;
}

// -------- interface impl --------

float URPGStatComponent::GetStat_Implementation(FGameplayTag Tag) const
{
	// Prefer Scalars; for Vitals return Current; for Skills return Level
	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE)
		return ScalarStats.Items[i].Value;
	if (int32 i = FindVitalIdx(Tag); i != INDEX_NONE)
		return Vitals.Items[i].Current;
	if (int32 i = FindSkillIdx(Tag); i != INDEX_NONE)
		return (float)Skills.Items[i].Level;

	return 0.f;
}

void URPGStatComponent::SetStat_Implementation(FGameplayTag Tag, float NewValue, bool bClamp)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerSetStat(Tag, NewValue, bClamp);
		return;
	}

	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE)
	{
		FRPGScalarEntry& E = ScalarStats.Items[i];
		E.Value = NewValue;
		ScalarStats.MarkItemDirty(E);
		BroadcastScalar(E);
		return;
	}
	if (int32 i = FindVitalIdx(Tag); i != INDEX_NONE)
	{
		FRPGVitalEntry& E = Vitals.Items[i];
		E.Current = bClamp ? FMath::Clamp(NewValue, 0.f, E.Max) : NewValue;
		Vitals.MarkItemDirty(E);
		BroadcastVital(E);
		return;
	}
	if (int32 i = FindSkillIdx(Tag); i != INDEX_NONE)
	{
		FRPGSkillEntry& E = Skills.Items[i];
		E.Level = FMath::Max(0, (int32)NewValue);
		Skills.MarkItemDirty(E);
		BroadcastSkill(E);
		return;
	}

	// Stat didn’t exist → create scalar by default
	const int32 NewIdx = FindOrAddScalar(Tag, nullptr);
	ScalarStats.Items[NewIdx].Value = NewValue;
	ScalarStats.MarkItemDirty(ScalarStats.Items[NewIdx]);
	BroadcastScalar(ScalarStats.Items[NewIdx]);
}

void URPGStatComponent::AddToStat_Implementation(FGameplayTag Tag, float Delta, bool bClamp)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		ServerAddToStat(Tag, Delta, bClamp);
		return;
	}

	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE)
	{
		FRPGScalarEntry& E = ScalarStats.Items[i];
		E.Value += Delta;
		ScalarStats.MarkItemDirty(E);
		BroadcastScalar(E);
		return;
	}
	if (int32 i = FindVitalIdx(Tag); i != INDEX_NONE)
	{
		FRPGVitalEntry& E = Vitals.Items[i];
		E.Current = bClamp ? FMath::Clamp(E.Current + Delta, 0.f, E.Max) : (E.Current + Delta);
		Vitals.MarkItemDirty(E);
		BroadcastVital(E);
		return;
	}
	if (int32 i = FindSkillIdx(Tag); i != INDEX_NONE)
	{
		FRPGSkillEntry& E = Skills.Items[i];
		E.XP += Delta;

		// Simple level-up loop
		while (E.XP >= E.XPToNext && E.XPToNext > 0.f)
		{
			E.XP -= E.XPToNext;
			E.Level++;
			E.XPToNext = ComputeXPToNext(E.Level);
		}
		Skills.MarkItemDirty(E);
		BroadcastSkill(E);
		return;
	}

	// Unknown tag: create scalar and add
	const int32 NewIdx = FindOrAddScalar(Tag, nullptr);
	ScalarStats.Items[NewIdx].Value += Delta;
	ScalarStats.MarkItemDirty(ScalarStats.Items[NewIdx]);
	BroadcastScalar(ScalarStats.Items[NewIdx]);
}

// -------- server RPCs --------

void URPGStatComponent::ServerSetStat_Implementation(FGameplayTag Tag, float NewValue, bool bClamp)
{
	SetStat_Implementation(Tag, NewValue, bClamp);
}

void URPGStatComponent::ServerAddToStat_Implementation(FGameplayTag Tag, float Delta, bool bClamp)
{
	AddToStat_Implementation(Tag, Delta, bClamp);
}

// -------- seed from data assets --------

void URPGStatComponent::InitializeFromStatSets(bool bClearExisting)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (bClearExisting)
	{
		ScalarStats.Items.Reset();
		Vitals.Items.Reset();
		Skills.Items.Reset();
	}

	for (UStatSetDataAsset* Set : StatSets)
	{
		if (!IsValid(Set)) continue;

		for (const FScalarDef& S : Set->Scalars)
		{
			const int32 Idx = FindOrAddScalar(S.Tag, &S.Name);
			ScalarStats.Items[Idx].Value = S.DefaultValue;
			ScalarStats.MarkItemDirty(ScalarStats.Items[Idx]);
		}
		for (const FVitalDef& V : Set->Vitals)
		{
			const int32 Idx = FindOrAddVital(V.Tag, &V.Name, V.DefaultCurrent, V.Max);
			FRPGVitalEntry& E = Vitals.Items[Idx];
			E.Current = FMath::Clamp(E.Current, 0.f, E.Max);
			Vitals.MarkItemDirty(E);
		}
		for (const FSkillDef& K : Set->Skills)
		{
			const int32 Idx = FindOrAddSkill(K.Tag, &K.Name, K.DefaultLevel);
			FRPGSkillEntry& E = Skills.Items[Idx];
			E.XP = K.StartingXP;
			E.XPToNext = (K.XPToNextOverride > 0.f) ? K.XPToNextOverride : ComputeXPToNext(E.Level);
			Skills.MarkItemDirty(E);
		}
	}

	bInitializedFromSets = true;
	RebuildCaches();
}

// -------- utils --------

bool URPGStatComponent::HasStat(FGameplayTag Tag) const
{
	return FindScalarIdx(Tag) != INDEX_NONE || FindVitalIdx(Tag) != INDEX_NONE || FindSkillIdx(Tag) != INDEX_NONE;
}

void URPGStatComponent::GetAllStatTags(TArray<FGameplayTag>& OutTags) const
{
	OutTags.Reserve(ScalarStats.Items.Num() + Vitals.Items.Num() + Skills.Items.Num());
	for (const auto& E : ScalarStats.Items) OutTags.Add(E.Tag);
	for (const auto& E : Vitals.Items)      OutTags.Add(E.Tag);
	for (const auto& E : Skills.Items)      OutTags.Add(E.Tag);
}

float URPGStatComponent::ComputeXPToNext(int32 Level) const
{
	// simple curve: grows moderately by level
	return 100.f + 25.f * FMath::Max(0, Level - 1);
}

void URPGStatComponent::BroadcastScalar(const FRPGScalarEntry& E) const
{
	if (GetOwner() && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		OnScalarChanged.Broadcast(E.Tag, E.Value);
	}
}
void URPGStatComponent::BroadcastVital(const FRPGVitalEntry& E) const
{
	if (GetOwner() && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		OnVitalChanged.Broadcast(E.Tag, E.Current, E.Max);
	}
}
void URPGStatComponent::BroadcastSkill(const FRPGSkillEntry& E) const
{
	if (GetOwner() && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		OnSkillChanged.Broadcast(E.Tag, E.Level, E.XP);
	}
}
