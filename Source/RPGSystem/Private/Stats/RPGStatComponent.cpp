#include "Stats/RPGStatComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// Define the helper now that the full component is visible.
void RPGStat_NetPostReplicated(URPGStatComponent* Owner)
{
	if (Owner)
	{
		Owner->RebuildCaches();
	}
}

URPGStatComponent::URPGStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Register owners for FastArray lists (so PostReplicated* can call back)
	ScalarStats.Register(this);
	Vitals     .Register(this);
	Skills     .Register(this);
}

void URPGStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URPGStatComponent, ScalarStats);
	DOREPLIFETIME(URPGStatComponent, Vitals);
	DOREPLIFETIME(URPGStatComponent, Skills);
}

void URPGStatComponent::RebuildCaches()
{
	ScalarIndex.Reset();
	VitalIndex .Reset();
	SkillIndex .Reset();

	for (int32 i = 0; i < ScalarStats.Items.Num(); ++i)
	{
		ScalarIndex.Add(ScalarStats.Items[i].Tag, i);
		BroadcastScalar(ScalarStats.Items[i]);
	}
	for (int32 i = 0; i < Vitals.Items.Num(); ++i)
	{
		VitalIndex.Add(Vitals.Items[i].Tag, i);
		BroadcastVital(Vitals.Items[i]);
	}
	for (int32 i = 0; i < Skills.Items.Num(); ++i)
	{
		SkillIndex.Add(Skills.Items[i].Tag, i);
		BroadcastSkill(Skills.Items[i]);
	}
}

void URPGStatComponent::InitializeFromStatSets(bool bClearExisting)
{
	if (bClearExisting)
	{
		ScalarStats.Items.Reset();
		Vitals     .Items.Reset();
		Skills     .Items.Reset();
	}

	// Scalars
	for (const UStatSetDataAsset* Set : InitialStatSets)
	{
		if (!Set) continue;

		for (const FRPGScalarDef& Def : Set->Modifiers)
		{
			if (!Def.Tag.IsValid()) continue;

			int32 Idx = FindScalarIdx(Def.Tag);
			if (Idx == INDEX_NONE)
			{
				FRPGScalarEntry& E = ScalarStats.Items.AddDefaulted_GetRef();
				E.Tag   = Def.Tag;
				E.Value = Def.DefaultValue;
				ScalarStats.MarkItemDirty(E);
			}
			else
			{
				FRPGScalarEntry& E = ScalarStats.Items[Idx];
				E.Value = Def.DefaultValue;
				ScalarStats.MarkItemDirty(E);
			}
		}

		// Vitals
		for (const FRPGVitalDef& Def : Set->Vitals)
		{
			if (!Def.Tag.IsValid()) continue;

			int32 Idx = INDEX_NONE;
			for (int32 i = 0; i < Vitals.Items.Num(); ++i)
				if (Vitals.Items[i].Tag == Def.Tag) { Idx = i; break; }

			if (Idx == INDEX_NONE)
			{
				FRPGVitalEntry& E = Vitals.Items.AddDefaulted_GetRef();
				E.Tag = Def.Tag;
				E.Max = FMath::Max(0.f, Def.DefaultMax);
				E.Current = FMath::Clamp(Def.DefaultCurrent, 0.f, E.Max);
				Vitals.MarkItemDirty(E);
			}
			else
			{
				FRPGVitalEntry& E = Vitals.Items[Idx];
				E.Max     = FMath::Max(0.f, Def.DefaultMax);
				E.Current = FMath::Clamp(Def.DefaultCurrent, 0.f, E.Max);
				Vitals.MarkItemDirty(E);
			}
		}

		// Skills
		for (const FRPGSkillDef& Def : Set->Skills)
		{
			if (!Def.Tag.IsValid()) continue;

			int32 Idx = INDEX_NONE;
			for (int32 i = 0; i < Skills.Items.Num(); ++i)
				if (Skills.Items[i].Tag == Def.Tag) { Idx = i; break; }

			if (Idx == INDEX_NONE)
			{
				FRPGSkillEntry& E = Skills.Items.AddDefaulted_GetRef();
				E.Tag     = Def.Tag;
				E.Level   = FMath::Max(0, Def.DefaultLevel);
				E.XP      = FMath::Max(0.f, Def.DefaultXP);
				E.XPToNext= FMath::Max(1.f, Def.DefaultXPToNext);
				Skills.MarkItemDirty(E);
			}
			else
			{
				FRPGSkillEntry& E = Skills.Items[Idx];
				E.Level   = FMath::Max(0, Def.DefaultLevel);
				E.XP      = FMath::Max(0.f, Def.DefaultXP);
				E.XPToNext= FMath::Max(1.f, Def.DefaultXPToNext);
				Skills.MarkItemDirty(E);
			}
		}
	}

	RebuildCaches();
}

// ---------- Interface ----------
float URPGStatComponent::GetStat_Implementation(FGameplayTag Tag, float DefaultValue) const
{
	if (!Tag.IsValid()) return DefaultValue;

	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE) return ScalarStats.Items[i].Value;
	if (int32 i = FindVitalIdx (Tag); i != INDEX_NONE) return Vitals     .Items[i].Current;
	if (int32 i = FindSkillIdx (Tag); i != INDEX_NONE) return (float)Skills.Items[i].Level;

	return DefaultValue;
}

void URPGStatComponent::SetStat_Implementation(FGameplayTag Tag, float NewValue)
{
	if (!Tag.IsValid()) return;

	// Scalars
	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE)
	{
		FRPGScalarEntry& E = ScalarStats.Items[i];
		if (!FMath::IsNearlyEqual(E.Value, NewValue))
		{
			E.Value = NewValue;
			ScalarStats.MarkItemDirty(E);
			BroadcastScalar(E);
		}
		return;
	}

	// Vitals (Current)
	if (int32 i = FindVitalIdx(Tag); i != INDEX_NONE)
	{
		FRPGVitalEntry& E = Vitals.Items[i];
		float NewCur = FMath::Clamp(NewValue, 0.f, FMath::Max(0.f, E.Max));
		if (!FMath::IsNearlyEqual(E.Current, NewCur))
		{
			E.Current = NewCur;
			Vitals.MarkItemDirty(E);
			BroadcastVital(E);
		}
		return;
	}

	// Skills (Level)
	if (int32 i = FindSkillIdx(Tag); i != INDEX_NONE)
	{
		FRPGSkillEntry& E = Skills.Items[i];
		int32 NewLevel = FMath::Max(0, (int32)FMath::RoundToInt(NewValue));
		if (E.Level != NewLevel)
		{
			E.Level = NewLevel;
			Skills.MarkItemDirty(E);
			BroadcastSkill(E);
		}
	}
}

void URPGStatComponent::AddToStat_Implementation(FGameplayTag Tag, float Delta)
{
	if (!Tag.IsValid() || FMath::IsNearlyZero(Delta)) return;

	// Scalars
	if (int32 i = FindScalarIdx(Tag); i != INDEX_NONE)
	{
		FRPGScalarEntry& E = ScalarStats.Items[i];
		E.Value += Delta;
		ScalarStats.MarkItemDirty(E);
		BroadcastScalar(E);
		return;
	}

	// Vitals (Current)
	if (int32 i = FindVitalIdx(Tag); i != INDEX_NONE)
	{
		FRPGVitalEntry& E = Vitals.Items[i];
		E.Current = FMath::Clamp(E.Current + Delta, 0.f, FMath::Max(0.f, E.Max));
		Vitals.MarkItemDirty(E);
		BroadcastVital(E);
		return;
	}

	// Skills (XP)
	if (int32 i = FindSkillIdx(Tag); i != INDEX_NONE)
	{
		FRPGSkillEntry& E = Skills.Items[i];
		E.XP = FMath::Max(0.f, E.XP + Delta);

		// Simple levelling: roll over while XP >= XPToNext
		while (E.XP >= E.XPToNext)
		{
			E.XP -= E.XPToNext;
			E.Level++;
			E.XPToNext = FMath::Max(1.f, E.XPToNext * 1.15f);
		}

		Skills.MarkItemDirty(E);
		BroadcastSkill(E);
	}
}
