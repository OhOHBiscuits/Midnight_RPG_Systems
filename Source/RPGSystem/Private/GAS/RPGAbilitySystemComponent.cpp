// RPGAbilitySystemComponent.cpp

#include "GAS/RPGAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"

URPGAbilitySystemComponent::URPGAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

URPGAbilitySystemComponent* URPGAbilitySystemComponent::AddTo(AActor* Owner, FName ComponentName)
{
	if (!Owner) return nullptr;

	if (URPGAbilitySystemComponent* Existing = Owner->FindComponentByClass<URPGAbilitySystemComponent>())
	{
		return Existing;
	}

	// Create a runtime component and register it
	URPGAbilitySystemComponent* ASC = NewObject<URPGAbilitySystemComponent>(Owner, ComponentName);
	if (!ASC) return nullptr;

	ASC->SetIsReplicated(true);

	// Make sure the Actor actually owns and registers the component properly
	Owner->AddOwnedComponent(ASC);
	ASC->OnComponentCreated();
	ASC->RegisterComponent();

	return ASC;
}

void URPGAbilitySystemComponent::InitializeForActor(AActor* InOwnerActor, AActor* InAvatarActor)
{
	// Safe no-ops if nulls
	if (!InOwnerActor)
	{
		InOwnerActor = GetOwner();
	}
	if (!InAvatarActor)
	{
		InAvatarActor = InOwnerActor;
	}

	InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

FGameplayEffectContextHandle URPGAbilitySystemComponent::MakeEffectContextWithSourceObject(UObject* InSourceObject)
{
	FGameplayEffectContextHandle Ctx = MakeEffectContext();
	if (AActor* Owner = GetOwnerActor())
	{
		Ctx.AddInstigator(Owner, GetAvatarActor());
	}
	if (InSourceObject)
	{
		Ctx.AddSourceObject(InSourceObject);
	}
	return Ctx;
}

FActiveGameplayEffectHandle URPGAbilitySystemComponent::ApplyGEWithSetByCallerToSelf(
	TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag                 SetByCallerTag,
	float                        Magnitude,
	float                        EffectLevel,
	UObject*                     SourceObject,
	int32                        Stacks)
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Ctx = MakeEffectContextWithSourceObject(SourceObject);

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, EffectLevel, Ctx);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	if (SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
	}
	// Fix deprecation: use SetStackCount instead of direct member
	SpecHandle.Data->SetStackCount(FMath::Max(1, Stacks));

	return ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

FActiveGameplayEffectHandle URPGAbilitySystemComponent::ApplyGEWithSetByCallerToTarget(
	AActor*                      TargetActor,
	TSubclassOf<UGameplayEffect> EffectClass,
	FGameplayTag                 SetByCallerTag,
	float                        Magnitude,
	float                        EffectLevel,
	UObject*                     SourceObject,
	int32                        Stacks,
	AActor*                      Instigator,
	AActor*                      EffectCauser)
{
	if (!TargetActor || !EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return FActiveGameplayEffectHandle();
	}

	// Use an ASC to author the spec: prefer Instigator's ASC, else Target ASC
	UAbilitySystemComponent* SourceASC = nullptr;
	if (Instigator)
	{
		SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator);
	}
	if (!SourceASC)
	{
		SourceASC = TargetASC;
	}

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	{
		AActor* FinalInstigator = Instigator ? Instigator : SourceASC->GetOwnerActor();
		AActor* FinalCauser     = EffectCauser ? EffectCauser : SourceASC->GetAvatarActor();
		if (FinalInstigator)
		{
			Ctx.AddInstigator(FinalInstigator, FinalCauser);
		}
		if (SourceObject)
		{
			Ctx.AddSourceObject(SourceObject);
		}
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, EffectLevel, Ctx);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	if (SetByCallerTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);
	}
	// Fix deprecation: use SetStackCount instead of direct member
	SpecHandle.Data->SetStackCount(FMath::Max(1, Stacks));

	return SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}
