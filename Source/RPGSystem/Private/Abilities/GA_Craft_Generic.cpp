#include "Abilities/GA_Craft_Generic.h"

#include "Crafting/CraftingRecipeDataAsset.h"
#include "Crafting/CraftingStationComponent.h"

UGA_Craft_Generic::UGA_Craft_Generic()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Craft_Generic::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UCraftingRecipeDataAsset* Recipe =
		TriggerEventData ? Cast<UCraftingRecipeDataAsset>(TriggerEventData->OptionalObject) : nullptr;

	AActor* StationActor =
		TriggerEventData ? const_cast<AActor*>(Cast<AActor>(TriggerEventData->OptionalObject2)) : nullptr;

	if (!Recipe || !DoCraft_Internal(Recipe, StationActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicate*/true, /*bWasCancelled*/true);
	}
}

bool UGA_Craft_Generic::DoCraft_Internal(const UCraftingRecipeDataAsset* Recipe, AActor* StationActor)
{
	if (!CommitCheck(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return false;
	}
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return false;
	}

	UCraftingStationComponent* Station = nullptr;

	if (StationActor)
	{
		Station = StationActor->FindComponentByClass<UCraftingStationComponent>();
	}
	else if (AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr)
	{
		// allow “hand crafting” if the avatar carries a station component
		Station = Avatar->FindComponentByClass<UCraftingStationComponent>();
	}

	if (!Station)
	{
		return false;
	}

	BoundStation = Station;

	Station->OnCraftStarted.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftStarted);
	Station->OnCraftCompleted.AddDynamic(this, &UGA_Craft_Generic::OnStationCraftCompleted);

	return Station->StartCraftFromRecipe(CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr,
	                                     const_cast<UCraftingRecipeDataAsset*>(Recipe));
}

void UGA_Craft_Generic::OnStationCraftStarted(const FCraftingRequest& /*Request*/, float /*FinalTime*/, const FSkillCheckResult& /*Check*/)
{
	// optional: play montage / show UI
}

void UGA_Craft_Generic::OnStationCraftCompleted(const FCraftingRequest& /*Request*/, const FSkillCheckResult& /*Check*/, bool bSuccess)
{
	ClearBindings();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicate*/true, /*bWasCancelled*/!bSuccess);
}

void UGA_Craft_Generic::ClearBindings()
{
	if (UCraftingStationComponent* S = BoundStation.Get())
	{
		S->OnCraftStarted.RemoveDynamic(this, &UGA_Craft_Generic::OnStationCraftStarted);
		S->OnCraftCompleted.RemoveDynamic(this, &UGA_Craft_Generic::OnStationCraftCompleted);
	}
	BoundStation.Reset();
}

void UGA_Craft_Generic::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                   const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                   bool bReplicateEndAbility,
                                   bool bWasCancelled)
{
	ClearBindings();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
