#include "Abilities/GA_Craft_Generic.h"
#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

void UGA_Craft_Generic::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEnd*/true, /*bWasCancelled*/true);
		return;
	}

	// Find a station on the avatar (you can change this lookup as needed)
	UCraftingStationComponent* Station = Avatar->FindComponentByClass<UCraftingStationComponent>();
	if (!Station)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Bind delegates
	if (!Station->OnCraftStarted.IsAlreadyBound(this, &UGA_Craft_Generic::OnStationCraftStarted))
	{
		Station->OnCraftStarted.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftStarted);
	}
	if (!Station->OnCraftFinished.IsAlreadyBound(this, &UGA_Craft_Generic::OnStationCraftFinished))
	{
		Station->OnCraftFinished.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftFinished);
	}
	BoundStation = Station;

	// Kick off crafting from event data if a recipe was sent
	if (TriggerEventData)
	{
		const UCraftingRecipeDataAsset* Recipe = Cast<UCraftingRecipeDataAsset>(TriggerEventData->OptionalObject);
		if (Recipe)
		{
			Station->StartCraftFromRecipe(Avatar, Recipe);
			return; // wait for callbacks
		}
	}

	// If no recipe provided, just end (or you could open UI)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

void UGA_Craft_Generic::OnStationCraftStarted(const FCraftingJob& /*Job*/)
{
	// Optionally: Play montage / SFX / lock input, etc.
}

void UGA_Craft_Generic::OnStationCraftFinished(const FCraftingJob& /*Job*/, bool bSuccess)
{
	// Unbind and end ability
	if (BoundStation)
	{
		BoundStation->OnCraftStarted.RemoveDynamic(this, &UGA_Craft_Generic::OnStationCraftStarted);
		BoundStation->OnCraftFinished.RemoveDynamic(this, &UGA_Craft_Generic::OnStationCraftFinished);
		BoundStation = nullptr;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEnd*/true, /*bWasCancelled*/!bSuccess);
}
