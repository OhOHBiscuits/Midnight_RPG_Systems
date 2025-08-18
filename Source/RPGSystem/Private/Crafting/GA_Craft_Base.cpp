#include "Crafting/GA_Craft_Base.h"
#include "Crafting/CraftingRecipeDataAsset.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/InventoryAssetManager.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryItem.h"

static FGameplayTag TAG_Event_Craft_Started   = FGameplayTag::RequestGameplayTag(FName("Event.Craft.Started"));
static FGameplayTag TAG_Event_Craft_Completed = FGameplayTag::RequestGameplayTag(FName("Event.Craft.Completed"));

UGA_Craft_Base::UGA_Craft_Base()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	bServerRespectsRemoteAbilityCancellation = true;
}

void UGA_Craft_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	HandleActivateFromEvent(*TriggerEventData);
}

void UGA_Craft_Base::HandleActivateFromEvent(const FGameplayEventData& EventData)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!Server_ValidateAndInit(EventData))
	{
		OnCraftCompleted.Broadcast(ActiveRecipe ? ActiveRecipe->RecipeIDTag : FGameplayTag(), RequestedQuantity, false);

		FGameplayEventData Completed;
		Completed.EventTag = TAG_Event_Craft_Completed;
		if (ActiveRecipe && ActiveRecipe->RecipeIDTag.IsValid()) Completed.TargetTags.AddTag(ActiveRecipe->RecipeIDTag);
		Completed.EventMagnitude = 0.f;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), Completed.EventTag, Completed);

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	OnCraftStarted.Broadcast(ActiveRecipe->RecipeIDTag, RequestedQuantity, true);

	FGameplayEventData Started;
	Started.EventTag = TAG_Event_Craft_Started;
	Started.EventMagnitude = (float)RequestedQuantity;
	if (ActiveRecipe && ActiveRecipe->RecipeIDTag.IsValid()) Started.TargetTags.AddTag(ActiveRecipe->RecipeIDTag);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), Started.EventTag, Started);

	Server_StartCraft();
}

bool UGA_Craft_Base::Server_ValidateAndInit(const FGameplayEventData& EventData)
{
	ActiveRecipe      = Cast<UCraftingRecipeDataAsset>(EventData.OptionalObject);

	// OptionalObject2 may be 'const'; strip const to assign into TWeakObjectPtr<AActor>
	if (const AActor* WSConst = Cast<AActor>(EventData.OptionalObject2))
	{
		ActiveWorkstation = const_cast<AActor*>(WSConst);
	}
	else
	{
		ActiveWorkstation = nullptr;
	}

	RequestedQuantity = FMath::Clamp((int32)FMath::FloorToInt(EventData.EventMagnitude), 1, 9999);
	CachedInventory   = ResolveInventory();

	if (!ActiveRecipe || !ActiveRecipe->IsValidRecipe()) return false;
	if (!CachedInventory.IsValid()) return false;

	if (GlobalCraftingEnableTag.IsValid())
	{
		if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid() ||
			!CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(GlobalCraftingEnableTag))
		{
			return false;
		}
	}

	if (!CheckUnlockTag())    return false;
	if (!CheckStationGates()) return false;
	if (!HasSufficientInputs(RequestedQuantity)) return false;

	return true;
}

bool UGA_Craft_Base::CheckUnlockTag() const
{
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid()) return false;
	if (!ActiveRecipe) return false;

	if (!ActiveRecipe->UnlockTag.IsValid()) return true;
	return CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(ActiveRecipe->UnlockTag);
}

bool UGA_Craft_Base::CheckStationGates() const
{
	// Extend with a station interface later if needed.
	return true;
}

UInventoryComponent* UGA_Craft_Base::ResolveInventory() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	return UInventoryHelpers::GetInventoryComponent(Avatar);
}

int32 UGA_Craft_Base::CountTotalByItemID(UInventoryComponent* Inv, const FGameplayTag& ItemID) const
{
	if (!Inv || !ItemID.IsValid()) return 0;

	// Use the public getter; assume stacking per ItemID (returns a single stack)
	const FInventoryItem Item = Inv->GetItemByID(ItemID);
	return Item.IsValid() ? Item.Quantity : 0;
}

bool UGA_Craft_Base::HasSufficientInputs(int32 InQuantity) const
{
	UInventoryComponent* Inv = CachedInventory.Get();
	if (!Inv) return false;

	for (const auto& Req : ActiveRecipe->Inputs)
	{
		const int32 Need = Req.Quantity * InQuantity;
		const int32 Have = CountTotalByItemID(Inv, Req.ItemIDTag);
		if (Have < Need) return false;
	}
	return true;
}

float UGA_Craft_Base::ComputeFinalTimeSeconds(int32 InQuantity) const
{
	float Time = FMath::Max(0.01f, ActiveRecipe->BaseTimeSeconds) * FMath::Max(1, InQuantity);

	if (AActor* WS = ActiveWorkstation.Get())
	{
		static const FName FuncName("ApplyEfficiencyToCost");
		if (UFunction* Fn = WS->FindFunction(FuncName))
		{
			struct { float In; float Out; } Params{ Time, 0.f };
			WS->ProcessEvent(Fn, &Params);
			if (Params.Out > 0.0f) Time = Params.Out;
		}
	}
	return Time;
}

void UGA_Craft_Base::TryPlayMontage(float DurationSeconds)
{
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid()) return;
	if (!ActiveRecipe) return;

	if (!ActiveRecipe->CraftMontage.IsValid() && ActiveRecipe->CraftMontage.IsNull()) return;

	UAnimMontage* Montage = ActiveRecipe->CraftMontage.IsValid()
		? ActiveRecipe->CraftMontage.Get()
		: ActiveRecipe->CraftMontage.LoadSynchronous();
	if (!Montage) return;

	const float Length = Montage->GetPlayLength();
	const float PlayRate = (Length > KINDA_SMALL_NUMBER) ? (Length / FMath::Max(0.01f, DurationSeconds)) : 1.f;

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, PlayRate, ActiveRecipe->MontageSectionName, true, 1.0f
	);
	Task->ReadyForActivation();
}

void UGA_Craft_Base::Server_StartCraft()
{
	const float Duration = ComputeFinalTimeSeconds(RequestedQuantity);
	TryPlayMontage(Duration);

	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	Wait->OnFinish.AddDynamic(this, &UGA_Craft_Base::Server_FinishCraft);
	Wait->ReadyForActivation();
}

bool UGA_Craft_Base::ConsumeInputs(UInventoryComponent* Inv, int32 InQuantity) const
{
	if (!Inv) return false;

	for (const auto& Req : ActiveRecipe->Inputs)
	{
		const int32 Need = Req.Quantity * InQuantity;
		if (!Inv->RemoveItemByID(Req.ItemIDTag, Need)) return false;
	}
	return true;
}

bool UGA_Craft_Base::GrantOutputs(UInventoryComponent* Inv, int32 InQuantity) const
{
	if (!Inv) return false;

	for (const auto& Out : ActiveRecipe->Outputs)
	{
		const int32 Give = Out.Quantity * InQuantity;
		if (UItemDataAsset* DA = UInventoryAssetManager::Get().LoadItemDataByTag(Out.ItemIDTag, true))
		{
			if (!Inv->AddItem(DA, Give))
			{
				if (!Inv->TryAddItem(DA, Give)) return false;
			}
		}
		else return false;
	}
	return true;
}

void UGA_Craft_Base::Server_FinishCraft()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UInventoryComponent* Inv = CachedInventory.Get();

	auto SendCompleted = [&](float Magnitude)
	{
		FGameplayEventData Completed;
		Completed.EventTag = TAG_Event_Craft_Completed;
		Completed.EventMagnitude = Magnitude; // 1=success, 0=failure
		if (ActiveRecipe && ActiveRecipe->RecipeIDTag.IsValid()) Completed.TargetTags.AddTag(ActiveRecipe->RecipeIDTag);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), Completed.EventTag, Completed);
	};

	if (!Inv)
	{
		OnCraftCompleted.Broadcast(FGameplayTag(), RequestedQuantity, false);
		SendCompleted(0.f);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!HasSufficientInputs(RequestedQuantity) || !ConsumeInputs(Inv, RequestedQuantity) || !GrantOutputs(Inv, RequestedQuantity))
	{
		OnCraftCompleted.Broadcast(ActiveRecipe ? ActiveRecipe->RecipeIDTag : FGameplayTag(), RequestedQuantity, false);
		SendCompleted(0.f);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// TODO: Apply reward GEs for Skill/Character XP via SetByCaller.

	OnCraftCompleted.Broadcast(ActiveRecipe->RecipeIDTag, RequestedQuantity, true);
	SendCompleted(1.f);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
