// Source/RPGSystem/Private/GAS/RPGCue_StatNotify.cpp
#include "GAS/RPGCue_StatNotify.h"
#include "Stats/StatProviderInterface.h"
#include "Components/ActorComponent.h"

static UObject* FindStatProviderOn(AActor* Target)
{
	if (!Target) return nullptr;

	if (Target->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
		return Target;

	TArray<UActorComponent*> Comps;
	Target->GetComponents(Comps);
	for (UActorComponent* C : Comps)
	{
		if (C && C->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
			return C;
	}
	return nullptr;
}

bool URPGCue_StatNotify::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	if (!StatTag.IsValid())
	{
		OnStatRead(Target, 0.f);
		return true;
	}

	UObject* Provider = FindStatProviderOn(Target);
	const float V = Provider ? IStatProviderInterface::Execute_GetStat(Provider, StatTag, 0.f) : 0.f;
	OnStatRead(Target, V);
	return true;
}
