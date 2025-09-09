// Source/RPGSystem/Private/Progression/StatProgressionBridge.cpp
#include "Progression/StatProgressionBridge.h"

#include "Progression/XPGrantBundle.h"
#include "Progression/SkillProgressionData.h"

#include "Stats/StatProviderInterface.h"       // your interface
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

static const float KSmall = 1e-6f;

UObject* UStatProgressionBridge::FindStatProvider(UObject* Context)
{
	if (!Context) return nullptr;

	// 1) Directly on the object?
	if (Context->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
	{
		return Context;
	}

	// 2) If it's an actor, search its components
	if (const AActor* Actor = Cast<AActor>(Context))
	{
		TArray<UActorComponent*> Comps;
		Actor->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (C && C->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
			{
				return C;
			}
		}
	}

	return nullptr;
}

float UStatProgressionBridge::GetStat(UObject* Provider, const FGameplayTag& Tag, float DefaultValue)
{
	if (!Provider || !Tag.IsValid()) return DefaultValue;
	return IStatProviderInterface::Execute_GetStat(Provider, Tag, DefaultValue);
}

void UStatProgressionBridge::SetStat(UObject* Provider, const FGameplayTag& Tag, float NewValue)
{
	if (!Provider || !Tag.IsValid()) return;
	IStatProviderInterface::Execute_SetStat(Provider, Tag, NewValue);
}

float UStatProgressionBridge::ComputeNextThreshold(const USkillProgressionData* Skill, float NewLevel, float IncrementOverride)
{
	const float Inc = (IncrementOverride > 0.f) ? IncrementOverride : (Skill ? Skill->BaseIncrementPerLevel : 1000.f);
	const float Level = FMath::Max(1.f, NewLevel);
	return FMath::Max(1.f, Inc * Level);
}

bool UStatProgressionBridge::ApplyXPBundle(AActor* Target, UXPGrantBundle* Bundle)
{
	if (!Target || !Bundle) return false;

	bool bAny = false;
	for (const FSkillXPGrant& G : Bundle->Grants)
	{
		if (G.Skill && G.XPGain != 0.f)
		{
			bAny |= ApplySkillXP(Target, G.Skill, G.XPGain, G.IncrementOverride, G.bCarryRemainderOverride);
		}
	}
	return bAny;
}

bool UStatProgressionBridge::ApplySkillXP(AActor* Target, USkillProgressionData* Skill, float XPGain,
                                          float IncrementOverride, bool bCarryRemainderOverride)
{
	if (!Target || !Skill) return false;

	UObject* Provider = FindStatProvider(Target);
	if (!Provider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatProgressionBridge] No StatProvider on %s"), *GetNameSafe(Target));
		return false;
	}

	// Grab tags; if a tag is missing, allow auto-derivation from SkillTag (optional)
	FGameplayTag LevelTag    = Skill->LevelTag;
	FGameplayTag XPTag       = Skill->XPTag;
	FGameplayTag XPToNextTag = Skill->XPToNextTag;

	// OPTIONAL auto-derive: Skill.MySkill -> Skill.MySkill.Level / .XP / .XPToNext
	auto DeriveIfMissing = [](const FGameplayTag& Base, const TCHAR* Suffix) -> FGameplayTag
	{
		if (!Base.IsValid()) return FGameplayTag();
		const FString Derived = Base.ToString() + TEXT(".") + Suffix;
		return FGameplayTag::RequestGameplayTag(FName(*Derived), /*ErrorIfNotFound*/false);
	};

	if (!LevelTag.IsValid())    LevelTag    = DeriveIfMissing(Skill->SkillTag, TEXT("Level"));
	if (!XPTag.IsValid())       XPTag       = DeriveIfMissing(Skill->SkillTag, TEXT("XP"));
	if (!XPToNextTag.IsValid()) XPToNextTag = DeriveIfMissing(Skill->SkillTag, TEXT("XPToNext"));

	// Read current values
	float Level    = GetStat(Provider, LevelTag,    0.f);
	float XP       = GetStat(Provider, XPTag,       0.f);
	float XPToNext = GetStat(Provider, XPToNextTag, 0.f);

	// Establish rule inputs
	const bool bCarry = (bCarryRemainderOverride) ? true : Skill->bCarryRemainder;
	if (XPToNext <= KSmall)
	{
		XPToNext = ComputeNextThreshold(Skill, FMath::Max(1.f, Level + 1.f), IncrementOverride);
	}

	// Apply gain
	XP += XPGain;

	// Level-up(s)
	int32 LevelsGained = 0;
	while (XP >= XPToNext && XPToNext > KSmall)
	{
		LevelsGained++;
		if (bCarry)
		{
			XP -= XPToNext;
		}
		else
		{
			XP = 0.f;
		}

		Level += 1.f;
		XPToNext = ComputeNextThreshold(Skill, Level + 1.f, IncrementOverride);
	}

	// Write back
	SetStat(Provider, LevelTag,    Level);
	SetStat(Provider, XPTag,       FMath::Max(0.f, XP));
	SetStat(Provider, XPToNextTag, XPToNext);

	UE_LOG(LogTemp, Log, TEXT("[StatProgressionBridge] %s: +%0.2f XP -> Lvl %0.0f (%d up), XP=%0.2f / Next=%0.2f"),
	       *GetNameSafe(Skill), XPGain, Level, LevelsGained, XP, XPToNext);

	return true;
}
