// RPGStatComponent.cpp

#include "Stats/RPGStatComponent.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

// ---------------- FRPGStatList (FastArray) ----------------

bool FRPGStatList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize(Items, DeltaParms, *this);
}

void FRPGStatList::PostReplicatedAdd(const TArrayView<int32>& /*ChangedIndices*/, int32 /*FinalSize*/)
{
	if (Owner) { Owner->RebuildCache(); }
}

void FRPGStatList::PostReplicatedChange(const TArrayView<int32>& /*ChangedIndices*/, int32 /*FinalSize*/)
{
	if (Owner) { Owner->RebuildCache(); }
}

void FRPGStatList::PostReplicatedRemove(const TArrayView<int32>& /*ChangedIndices*/, int32 /*FinalSize*/)
{
	if (Owner) { Owner->RebuildCache(); }
}

// ---------------- URPGStatComponent ----------------

URPGStatComponent::URPGStatComponent()
{
	SetIsReplicatedByDefault(true);
}

void URPGStatComponent::OnRegister()
{
	Super::OnRegister();
	ReplicatedStats.Owner = this; // ensure FastArray callbacks can talk back
}

void URPGStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URPGStatComponent, ReplicatedStats);
}

int32 URPGStatComponent::FindIndex(FGameplayTag Tag) const
{
	for (int32 i = 0; i < ReplicatedStats.Items.Num(); ++i)
	{
		if (ReplicatedStats.Items[i].Tag == Tag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void URPGStatComponent::RebuildCache()
{
	Cache.Reset();
	for (const FRPGStatEntry& E : ReplicatedStats.Items)
	{
		if (E.Tag.IsValid())
		{
			Cache.Add(E.Tag, E.Value);
			// Broadcast on clients so UI updates when replication arrives
			OnStatChanged.Broadcast(E.Tag, E.Value);
		}
	}
}

float URPGStatComponent::GetStat(FGameplayTag Tag, float DefaultValue) const
{
	if (!Tag.IsValid())
	{
		return DefaultValue;
	}

	if (const float* Found = Cache.Find(Tag))
	{
		return *Found;
	}

	// Fallback if cache not built yet (e.g., first client tick):
	const int32 Idx = FindIndex(Tag);
	return (Idx != INDEX_NONE) ? ReplicatedStats.Items[Idx].Value : DefaultValue;
}

void URPGStatComponent::SetStat(FGameplayTag Tag, float NewValue)
{
	if (!Tag.IsValid())
	{
		return;
	}

	const bool bServer = GetOwner() && GetOwner()->HasAuthority();
	if (!bServer)
	{
		ServerSetStat(Tag, NewValue);
		return;
	}

	SetStat_Internal(Tag, NewValue);
}

void URPGStatComponent::AddToStat(FGameplayTag Tag, float Delta)
{
	if (!Tag.IsValid() || FMath::IsNearlyZero(Delta))
	{
		return;
	}

	const bool bServer = GetOwner() && GetOwner()->HasAuthority();
	if (!bServer)
	{
		ServerAddToStat(Tag, Delta);
		return;
	}

	const float Current = GetStat(Tag, 0.f);
	SetStat_Internal(Tag, Current + Delta);
}

void URPGStatComponent::SetStat_Internal(FGameplayTag Tag, float NewValue)
{
	int32 Idx = FindIndex(Tag);
	if (Idx == INDEX_NONE)
	{
		Idx = ReplicatedStats.Items.AddDefaulted();
		ReplicatedStats.Items[Idx].Tag = Tag;
	}

	ReplicatedStats.Items[Idx].Value = NewValue;

	// Tell the FastArray we changed one element
	ReplicatedStats.MarkItemDirty(ReplicatedStats.Items[Idx]);

	// Keep the cache fresh server-side too
	Cache.Add(Tag, NewValue);

	// Server broadcast (clients will also broadcast in RebuildCache when replication lands)
	OnStatChanged.Broadcast(Tag, NewValue);
}

// ---------------- RPCs ----------------

void URPGStatComponent::ServerSetStat_Implementation(FGameplayTag Tag, float NewValue)
{
	SetStat_Internal(Tag, NewValue);
}

void URPGStatComponent::ServerAddToStat_Implementation(FGameplayTag Tag, float Delta)
{
	const float Current = GetStat(Tag, 0.f);
	SetStat_Internal(Tag, Current + Delta);
}
