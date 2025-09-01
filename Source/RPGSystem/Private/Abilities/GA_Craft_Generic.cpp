#include "Abilities/GA_Craft_Generic.h"
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

void UGA_Craft_Generic::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	const UCraftingRecipeDataAsset* Recipe =
		Cast<UCraftingRecipeDataAsset>(TriggerEventData ? TriggerEventData->OptionalObject : nullptr);

	const AActor* StationActorConst =
		Cast<AActor>(TriggerEventData ? TriggerEventData->OptionalObject2 : nullptr);

	AActor* StationActor = const_cast<AActor*>(StationActorConst);
	if (!Recipe || !StationActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UCraftingStationComponent* Station = StationActor->FindComponentByClass<UCraftingStationComponent>())
	{
		// Bind UI/flow hooks
		Station->OnCraftStarted.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftStarted);
		Station->OnCraftFinished.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftFinished);

		AActor* Instigator = ActorInfo->AvatarActor.Get();
		Station->StartCraftFromRecipe(Instigator, Recipe);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Craft_Generic::OnStationCraftStarted(const FCraftingJob& /*Job*/)
{
	// Optional: gameplay cues, UI updates, etc.
}

void UGA_Craft_Generic::OnStationCraftFinished(const FCraftingJob& /*Job*/, bool bSuccess)
{
	// Unbind (safety) – find the station off the job owner if you keep a pointer, or rely on EndAbility cleanup
	// Here we just end the ability.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicate*/true, /*bWasCancelled*/!bSuccess);
}
