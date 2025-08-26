#include "Progression/ProgressionBlueprintLibrary.h"
#include "Progression/SkillProgressionData.h"
#include "Progression/XPGrantBundle.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	static FGameplayTag TAG_XP       = FGameplayTag::RequestGameplayTag(FName("Data.XP"));
	static FGameplayTag TAG_XP_INC   = FGameplayTag::RequestGameplayTag(FName("Data.XP.Increment"));
	static FGameplayTag TAG_XP_CARRY = FGameplayTag::RequestGameplayTag(FName("Data.XP.CarryRemainder"));
}

UAbilitySystemComponent* UProgressionBlueprintLibrary::ResolveASCFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;

	// 1) Direct lookup on this Actor (works for PlayerState if passed directly)
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
	{
		return ASC;
	}

	// 2) Controller -> PlayerState -> ASC, then Controller -> Pawn -> ASC
	if (const AController* C = Cast<AController>(Actor))
	{
		if (APlayerState* PS = C->PlayerState)
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
				return ASC;

		if (APawn* P = C->GetPawn())
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P))
				return ASC;
	}

	// 3) Pawn -> PlayerState -> ASC
	if (const APawn* P = Cast<APawn>(Actor))
	{
		if (APlayerState* PS = P->GetPlayerState())
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
				return ASC;
	}

	return nullptr;
}

bool UProgressionBlueprintLibrary::ApplySkillXP_GE(AActor* Instigator,
                                                   AActor* Target,
                                                   TSubclassOf<UGameplayEffect> AddXPEffectClass,
                                                   USkillProgressionData* SkillData,
                                                   float XPGain,
                                                   float IncrementOverride,
                                                   bool bCarryRemainderOverride)
{
	if (!Instigator || !Target || !AddXPEffectClass || !SkillData) return false;

	UAbilitySystemComponent* SourceASC = ResolveASCFromActor(Instigator);
	UAbilitySystemComponent* TargetASC = ResolveASCFromActor(Target);
	if (!SourceASC || !TargetASC) return false;

	// Server-only
	if (!SourceASC->GetOwner() || !SourceASC->GetOwner()->HasAuthority()) return false;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(SkillData);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(AddXPEffectClass, 1.f, Ctx);
	if (!Spec.IsValid()) return false;

	Spec.Data->SetSetByCallerMagnitude(TAG_XP, FMath::Max(0.f, XPGain));
	if (IncrementOverride > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_XP_INC, IncrementOverride);
	}
	if (bCarryRemainderOverride)
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_XP_CARRY, 1.f);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	return true;
}

int32 UProgressionBlueprintLibrary::ApplySkillXP_Bundle(AActor* Instigator,
                                                        AActor* Target,
                                                        TSubclassOf<UGameplayEffect> AddXPEffectClass,
                                                        UXPGrantBundle* Bundle)
{
	if (!Bundle) return 0;

	int32 Applied = 0;
	for (const FSkillXPGrant& G : Bundle->Grants)
	{
		if (G.Skill && G.XPGain > 0.f)
		{
			Applied += ApplySkillXP_GE(Instigator, Target, AddXPEffectClass,
			                           G.Skill, G.XPGain, G.IncrementOverride, G.bCarryRemainderOverride) ? 1 : 0;
		}
	}
	return Applied;
}
