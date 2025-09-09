// StatBlueprintLibrary.cpp
#include "Stats/StatBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"

static bool FindProviderOnObject(UObject* Obj, TScriptInterface<IStatProviderInterface>& Out)
{
	if (!Obj) return false;

	// Direct implementer?
	if (Obj->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
	{
		Out.SetObject(Obj);
		Out.SetInterface(Cast<IStatProviderInterface>(Obj));
		return true;
	}

	// Try components if it's an actor
	if (AActor* A = Cast<AActor>(Obj))
	{
		TArray<UActorComponent*> Comps;
		A->GetComponents(Comps);
		for (UActorComponent* C : Comps)
		{
			if (C && C->GetClass()->ImplementsInterface(UStatProviderInterface::StaticClass()))
			{
				Out.SetObject(C);
				Out.SetInterface(Cast<IStatProviderInterface>(C));
				return true;
			}
		}
	}
	return false;
}

bool URPGStatBlueprintLibrary::FindStatProvider(AActor* Actor, TScriptInterface<IStatProviderInterface>& OutProvider)
{
	if (!Actor) return false;

	// 1) Actor / its components
	if (FindProviderOnObject(Actor, OutProvider))
		return true;

	// 2) Pawn → PlayerState
	if (const APawn* P = Cast<APawn>(Actor))
	{
		if (FindProviderOnObject(P->GetPlayerState(), OutProvider))
			return true;

		// 3) Pawn → Controller
		if (FindProviderOnObject(P->GetController(), OutProvider))
			return true;
	}

	return false;
}

float URPGStatBlueprintLibrary::GetStatFromActor(AActor* Actor, FGameplayTag Tag, float DefaultValue)
{
	TScriptInterface<IStatProviderInterface> Prov;
	return FindStatProvider(Actor, Prov) ? IStatProviderInterface::Execute_GetStat(Prov.GetObject(), Tag, DefaultValue)
	                                     : DefaultValue;
}

void URPGStatBlueprintLibrary::SetStatOnActor(AActor* Actor, FGameplayTag Tag, float NewValue)
{
	TScriptInterface<IStatProviderInterface> Prov;
	if (FindStatProvider(Actor, Prov))
	{
		IStatProviderInterface::Execute_SetStat(Prov.GetObject(), Tag, NewValue);
	}
}

void URPGStatBlueprintLibrary::AddToStatOnActor(AActor* Actor, FGameplayTag Tag, float Delta)
{
	TScriptInterface<IStatProviderInterface> Prov;
	if (FindStatProvider(Actor, Prov))
	{
		IStatProviderInterface::Execute_AddToStat(Prov.GetObject(), Tag, Delta);
	}
}
