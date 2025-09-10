// GAS/RPGMMC_StatTag.cpp

#include "GAS/RPGMMC_StatTag.h"
#include "Progression/StatProgressionBridge.h"

#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"

URPGMMC_StatTag::URPGMMC_StatTag()
{
	// Pure lookup MMC; no capture defs needed.
}

float URPGMMC_StatTag::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	if (!StatToRead.IsValid())
	{
		return 0.f;
	}

	const FGameplayEffectContextHandle& Ctx = Spec.GetContext();

	UObject* Provider = nullptr;

	// 1) Effect Causer (often weapon or avatar)
	if (const AActor* Causer = Ctx.GetEffectCauser())
	{
		Provider = UStatProgressionBridge::FindStatProviderOn(Causer);
	}

	// 2) Optional: SourceObject (some effects set this to an Actor/DataAsset)
	if (!Provider)
	{
		if (UObject* SourceObj = Ctx.GetSourceObject())
		{
			if (const AActor* SourceActor = Cast<AActor>(SourceObj))
			{
				Provider = UStatProgressionBridge::FindStatProviderOn(SourceActor);
			}
			else
			{
				// If SourceObject itself implements the stat provider interface,
				// the bridge will handle it when we call GetStat below.
				Provider = SourceObj;
			}
		}
	}

	// Nothing found? Return 0 safely.
	if (!Provider)
	{
		return 0.f;
	}

	return UStatProgressionBridge::GetStat(Provider, StatToRead, /*Default*/ 0.f);
}
