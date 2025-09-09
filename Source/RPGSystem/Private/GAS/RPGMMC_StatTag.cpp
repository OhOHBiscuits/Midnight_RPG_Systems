#include "GAS/RPGMMC_StatTag.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Progression/StatProgressionBridge.h" // UStatProgressionBridge

// NOTE: Header for URPGMMC_StatTag should declare a UPROPERTY(EditDefaultsOnly)
// FGameplayTag StatToRead;  This cpp assumes that property exists.

float URPGMMC_StatTag::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Prefer reading stats from the source (instigator) side.
	const UAbilitySystemComponent* SourceASC =
		Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();

	UObject* Provider = nullptr;

	if (SourceASC && SourceASC->GetOwnerActor())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(SourceASC->GetOwnerActor());
	}

	// Fall back to effect causer (often the weapon or avatar) if needed.
	if (!Provider)
	{
		if (AActor* Causer = Spec.GetContext().GetEffectCauser())
		{
			Provider = UStatProgressionBridge::FindStatProviderOn(Causer);
		}
	}

	// If we still don’t have anything, just return 0 (or a default if you prefer).
	if (!Provider || !StatToRead.IsValid())
	{
		return 0.f;
	}

	return UStatProgressionBridge::GetStat(Provider, StatToRead, 0.f);
}
