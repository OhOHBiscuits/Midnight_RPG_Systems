#include "Progression/StatProgressionBridge.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

static bool ImplementsStatProvider(UObject* Obj)
{
	return Obj && Obj->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass());
}

float UStatProgressionBridge::GetStat(UObject* Provider, FGameplayTag Tag, float DefaultValue)
{
	if (!ImplementsStatProvider(Provider) || !Tag.IsValid())
	{
		return DefaultValue;
	}
	return IStatProviderInterface::Execute_GetStat(Provider, Tag, DefaultValue);
}

void UStatProgressionBridge::SetStat(UObject* Provider, FGameplayTag Tag, float NewValue)
{
	if (!ImplementsStatProvider(Provider) || !Tag.IsValid())
	{
		return;
	}
	IStatProviderInterface::Execute_SetStat(Provider, Tag, NewValue);
}

void UStatProgressionBridge::AddToStat(UObject* Provider, FGameplayTag Tag, float Delta)
{
	if (!ImplementsStatProvider(Provider) || !Tag.IsValid())
	{
		return;
	}
	IStatProviderInterface::Execute_AddToStat(Provider, Tag, Delta);
}

UObject* UStatProgressionBridge::FindStatProviderOn(AActor* Root)
{
	if (!Root) return nullptr;

	// 1) The actor itself
	if (ImplementsStatProvider(Root))
	{
		return Root;
	}

	// 2) Any components on the actor
	{
		TInlineComponentArray<UActorComponent*> Comps(Root);
		for (UActorComponent* C : Comps)
		{
			if (ImplementsStatProvider(C))
			{
				return C;
			}
		}
	}

	// 3) Controller (if any)
	if (AController* Ctrl = Root->GetInstigatorController())
	{
		if (ImplementsStatProvider(Ctrl))
		{
			return Ctrl;
		}

		TInlineComponentArray<UActorComponent*> CtrlComps(Ctrl);
		for (UActorComponent* C : CtrlComps)
		{
			if (ImplementsStatProvider(C))
			{
				return C;
			}
		}

		// 4) PlayerState (if any)
		if (APlayerState* PS = Ctrl->GetPlayerState<APlayerState>())
		{
			if (ImplementsStatProvider(PS))
			{
				return PS;
			}

			// PlayerState is an Actor; we can search its components as well.
			TInlineComponentArray<UActorComponent*> PSComps(static_cast<const AActor*>(PS));
			for (UActorComponent* C : PSComps)
			{
				if (ImplementsStatProvider(C))
				{
					return C;
				}
			}
		}
	}

	// 5) Walk up the owner chain if present
	if (AActor* Owner = Root->GetOwner())
	{
		if (UObject* OnOwner = FindStatProviderOn(Owner))
		{
			return OnOwner;
		}
	}

	return nullptr;
}

void UStatProgressionBridge::ApplyXPAndLevelUp(
	UObject* Provider,
	FGameplayTag LevelTag,
	FGameplayTag XPTag,
	FGameplayTag XPToNextTag,
	float DeltaXP,
	float MinLevel,
	float MaxLevel)
{
	if (!ImplementsStatProvider(Provider) || DeltaXP == 0.f)
	{
		return;
	}

	float Level    = GetStat(Provider, LevelTag,    0.f);
	float XP       = GetStat(Provider, XPTag,       0.f);
	float XPToNext = GetStat(Provider, XPToNextTag, 0.f);

	XP += DeltaXP;

	// Provide a default curve if XPToNext is not initialized
	auto DefaultThresholdForLevel = [](float Lvl) -> float
	{
		// very basic curve: grows linearly; replace with your DataAsset lookups when ready
		return FMath::Max(10.f, 100.f + (Lvl * 25.f));
	};

	if (XPToNext <= 0.f)
	{
		XPToNext = DefaultThresholdForLevel(Level);
	}

	// Level up while we have enough XP
	while (XP >= XPToNext)
	{
		XP -= XPToNext;
		Level = FMath::Clamp(Level + 1.f, MinLevel, MaxLevel);

		// Next threshold – here we just recompute by level.
		XPToNext = DefaultThresholdForLevel(Level);
	}

	// Write back results
	SetStat(Provider, LevelTag,    Level);
	SetStat(Provider, XPTag,       FMath::Max(0.f, XP));
	SetStat(Provider, XPToNextTag, FMath::Max(1.f, XPToNext));
}
