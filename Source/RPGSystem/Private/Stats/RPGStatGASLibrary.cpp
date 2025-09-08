#include "Stats/RPGStatGASLibrary.h"
#include "Stats/RPGStatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"              // <-- for GetAbilitySystemComponentFromActor
#include "AbilitySystemInterface.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"

// -------- helpers (private) ----------
static UAbilitySystemComponent* FindASCDeep(const AActor* Actor)
{
	if (!Actor) return nullptr;

	// Interface first (works for Pawn/PS/anything that implements it)
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			return ASC;
		}
	}

	// Component or interface on this actor
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		return ASC;
	}

	// If it's a pawn, try PS then Controller
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS))
			{
				return ASC;
			}
		}
		if (const AController* PC = Pawn->GetController())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PC))
			{
				return ASC;
			}
		}
	}

	// Walk owner chain once
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor->GetOwner());
}

static URPGStatComponent* FindStatsDeep(const AActor* Actor)
{
	if (!Actor) return nullptr;

	if (URPGStatComponent* C = Actor->FindComponentByClass<URPGStatComponent>())
	{
		return C;
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const APlayerState* PS = Pawn->GetPlayerState())
			if (URPGStatComponent* C = PS->FindComponentByClass<URPGStatComponent>())
				return C;

		if (const AController* PC = Pawn->GetController())
			if (URPGStatComponent* C = PC->FindComponentByClass<URPGStatComponent>())
				return C;
	}

	if (const AActor* Owner = Actor->GetOwner())
		if (URPGStatComponent* C = Owner->FindComponentByClass<URPGStatComponent>())
			return C;

	return nullptr;
}
// -------------------------------------

bool URPGStatGASLibrary::GetStatFromActor(AActor* FromActor, FGameplayTag StatTag, float& OutValue)
{
	OutValue = 0.f;
	if (!FromActor) return false;

	if (URPGStatComponent* C = FindStatsDeep(FromActor))
	{
		OutValue = C->GetStat(StatTag, 0.f);
		return true;
	}
	return false;
}

bool URPGStatGASLibrary::ApplyGE_WithSetByCallerFromStat(
	UAbilitySystemComponent* SourceASC,
	TSubclassOf<UGameplayEffect> GEClass,
	AActor* SourceActorForStat,
	FGameplayTag MagnitudeTag,
	AActor* TargetActor,
	float Multiplier)
{
	if (!SourceASC || !*GEClass || !TargetActor) return false;

	float Magnitude = 0.f;
	GetStatFromActor(SourceActorForStat ? SourceActorForStat : SourceASC->GetOwner(), MagnitudeTag, Magnitude);
	Magnitude *= Multiplier;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(SourceASC->GetOwner());

	FGameplayEffectSpecHandle SpecH = SourceASC->MakeOutgoingSpec(GEClass, 1.f, Ctx);
	if (!SpecH.IsValid()) return false;

	SpecH.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);

	if (UAbilitySystemComponent* TargetASC = FindASCDeep(TargetActor))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecH.Data.Get(), TargetASC);
		return true;
	}
	return false;
}

void URPGStatGASLibrary::ExecuteCue(UAbilitySystemComponent* ASC, FGameplayTag CueTag)
{
	if (!ASC || !CueTag.IsValid()) return;
	ASC->ExecuteGameplayCue(CueTag);
}
