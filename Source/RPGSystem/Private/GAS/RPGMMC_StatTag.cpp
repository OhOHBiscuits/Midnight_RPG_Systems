// Source/RPGSystem/Private/GAS/RPGMMC_StatTag.cpp
#include "GAS/RPGMMC_StatTag.h"
#include "AbilitySystemComponent.h"
#include "Stats/StatProviderInterface.h"

URPGMMC_StatTag::URPGMMC_StatTag()
{
}

UObject* URPGMMC_StatTag::FindProviderFromASC(const UAbilitySystemComponent* ASC)
{
	if (!ASC) return nullptr;

	// Directly on avatar or owner…
	auto TryFind = [](UObject* Obj) -> UObject*
	{
		if (!Obj) return nullptr;

		if (Obj->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
			return Obj;

		if (AActor* A = Cast<AActor>(Obj))
		{
			TArray<UActorComponent*> Comps;
			A->GetComponents(Comps);
			for (UActorComponent* C : Comps)
			{
				if (C && C->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
					return C;
			}
		}
		return nullptr;
	};

	if (UObject* P = TryFind(ASC->GetAvatarActor())) return P;
	if (UObject* P = TryFind(ASC->GetOwnerActor()))  return P;
	return nullptr;
}

float URPGMMC_StatTag::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	if (!StatTag.IsValid())
	{
		return DefaultValue;
	}

	const UAbilitySystemComponent* SourceASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = Spec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent(); // fallback approach

	const UAbilitySystemComponent* ASC = (StatFrom == ERPGStatSource::Source) ? SourceASC : Spec.GetTargetAbilitySystemComponent();

	UObject* Provider = FindProviderFromASC(ASC);
	if (!Provider)
	{
		// Try the other side as a last resort
		Provider = FindProviderFromASC(StatFrom == ERPGStatSource::Source ? Spec.GetTargetAbilitySystemComponent() : SourceASC);
	}
	if (!Provider) return DefaultValue;

	return IStatProviderInterface::Execute_GetStat(Provider, StatTag, DefaultValue);
}
