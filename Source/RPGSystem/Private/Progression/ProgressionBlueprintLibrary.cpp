#include "Progression/ProgressionBlueprintLibrary.h"
#include "Progression/SkillProgressionData.h"
#include "Progression/XPGrantBundle.h"
#include "Progression/ProgressionTags.h"   // <-- use native tags

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UAbilitySystemComponent* UProgressionBlueprintLibrary::ResolveASCFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor))
		return ASC;

	if (const AController* C = Cast<AController>(Actor))
	{
		if (APlayerState* PS = C->PlayerState)
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS))
				return ASC;

		if (APawn* P = C->GetPawn())
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(P))
				return ASC;
	}

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

	if (!SourceASC->GetOwner() || !SourceASC->GetOwner()->HasAuthority()) return false;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddSourceObject(SkillData);

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(AddXPEffectClass, 1.f, Ctx);
	if (!Spec.IsValid()) return false;

	Spec.Data->SetSetByCallerMagnitude(TAG_Data_XP, FMath::Max(0.f, XPGain));
	if (IncrementOverride > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_Data_XP_Increment, IncrementOverride);
	}
	if (bCarryRemainderOverride)
	{
		Spec.Data->SetSetByCallerMagnitude(TAG_Data_XP_CarryRemainder, 1.f);
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

void UProgressionBlueprintLibrary::InitSkillXPToNext(UAbilitySystemComponent* ASC, const USkillProgressionData* Skill)
{
	if (!ASC || !Skill) return;

	const int32 Level = FMath::FloorToInt(ASC->GetNumericAttribute(Skill->LevelAttribute));
	const float Threshold = Skill->ComputeXPToNext(Level);

	if (Skill->XPToNextAttribute.IsValid())
	{
		ASC->SetNumericAttributeBase(Skill->XPToNextAttribute, Threshold);
	}
}