#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "Inventory/InventoryComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagAssetInterface.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

UCraftingStationComponent::UCraftingStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingStationComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe) return false;

	FCraftingRequest Req;
	Req.RecipeID          = Recipe->RecipeIDTag;
	Req.Inputs            = Recipe->Inputs;
	Req.Outputs           = Recipe->Outputs;
	Req.BaseTimeSeconds   = Recipe->BaseTimeSeconds;
	Req.CheckDef          = Recipe->Check;                     // may be null -> we’ll use DefaultCheck
	Req.SkillForXP        = Recipe->SkillForXP;
	Req.XPGain            = Recipe->XPTotal;
	Req.XPOnStartFraction = Recipe->XPOnStartFraction;
	Req.PresencePolicy    = Recipe->PresencePolicy;
	Req.PresenceRadius    = Recipe->PresenceRadius;

	return StartCraft(Instigator, Req);
}

bool UCraftingStationComponent::StartCraft(AActor* Instigator, const FCraftingRequest& Request)
{
	if (bIsCrafting || !Instigator) return false;

	ActiveRequest = Request;
	if (!ActiveRequest.CheckDef && DefaultCheck)
	{
		ActiveRequest.CheckDef = DefaultCheck;
	}

	// === Skill Check ===
	// To avoid hard dependency on your BP function name, use a safe default here.
	// Hook your real check later (BP or C++) and set ActiveCheck there.
	ActiveCheck = FSkillCheckResult{};
	ActiveCheck.bSuccess       = true;      // default pass
	ActiveCheck.TimeMultiplier = 1.0f;      // no speed change
	ActiveCheck.QualityTier    = 0;         // baseline quality

	const float TimeMult = FMath::Max(0.01f, ActiveCheck.TimeMultiplier);
	const float FinalTime = FMath::Max(0.0f, ActiveRequest.BaseTimeSeconds * TimeMult);

	// Broadcast start
	FCraftingJob Job;
	Job.Request   = ActiveRequest;
	Job.FinalTime = FinalTime;
	OnCraftStarted.Broadcast(Job);

	// XP at start
	GiveStartXPIfAny(ActiveRequest, Instigator);

	// Timer
	bIsCrafting = true;
	CraftInstigator = Instigator;
	GetWorld()->GetTimerManager().SetTimer(CraftTimer, this, &UCraftingStationComponent::FinishCraft, FinalTime, false);

	return true;
}

void UCraftingStationComponent::CancelCraft()
{
	if (!bIsCrafting) return;

	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);

	FCraftingJob Job;
	Job.Request  = ActiveRequest;
	Job.FinalTime = 0.f;

	bIsCrafting = false;
	CraftInstigator = nullptr;

	OnCraftFinished.Broadcast(Job, false);
}

void UCraftingStationComponent::FinishCraft()
{
	AActor* Instigator = CraftInstigator.Get();
	const bool bPresenceOK = IsPresenceSatisfied(Instigator, ActiveRequest);
	const bool bSuccess = bPresenceOK && ActiveCheck.bSuccess;

	if (bSuccess)
	{
		const int32 QualityTier = ActiveCheck.QualityTier;
		GrantOutputs(ActiveRequest.Outputs, QualityTier, Instigator);
	}

	GiveFinishXPIfAny(ActiveRequest, Instigator, bSuccess);

	FCraftingJob Job;
	Job.Request   = ActiveRequest;
	Job.FinalTime = ActiveRequest.BaseTimeSeconds * FMath::Max(0.01f, ActiveCheck.TimeMultiplier);

	bIsCrafting = false;
	CraftInstigator = nullptr;

	OnCraftFinished.Broadcast(Job, bSuccess);
}

UInventoryComponent* UCraftingStationComponent::ResolveOutputInventory(AActor* Instigator) const
{
	if (AActor* Owner = GetOwner())
	{
		if (UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>())
		{
			return Inv;
		}
	}
	return Instigator ? Instigator->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UCraftingStationComponent::GrantOutputs(const TArray<FCraftItemOutput>& Outputs, int32 /*QualityTier*/, AActor* Instigator)
{
	if (Outputs.IsEmpty()) return;

	if (UInventoryComponent* OutInv = ResolveOutputInventory(Instigator))
	{
		// NOTE: Your InventoryComponent doesn’t expose AddItemByAsset.
		// Hook your project’s real add API here. For now we just log for visibility.
		for (const FCraftItemOutput& Out : Outputs)
		{
			if (!Out.Item || Out.Quantity <= 0) continue;
			UE_LOG(LogTemp, Log, TEXT("[Crafting] GrantOutputs -> %s x%d"),
				*GetNameSafe(Out.Item), Out.Quantity);
			// TODO: Replace with your Inventory API, e.g. OutInv->AddItemByTag(Out.Item->ItemTag, Out.Quantity);
		}
	}
}

void UCraftingStationComponent::GiveStartXPIfAny(const FCraftingRequest& Req, AActor* Instigator)
{
	if (!Req.SkillForXP || Req.XPGain <= 0.f) return;

	const float StartFrac = (Req.XPOnStartFraction < 0.f) ? DefaultXPOnStartFraction : Req.XPOnStartFraction;
	const float Amount = FMath::Clamp(StartFrac, 0.f, 1.f) * Req.XPGain;
	if (Amount <= 0.f) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	if (!Recipient) return;

	// TODO: Apply AddXPEffectClass to Recipient’s ASC for Amount
}

void UCraftingStationComponent::GiveFinishXPIfAny(const FCraftingRequest& Req, AActor* Instigator, bool /*bSuccess*/)
{
	if (!Req.SkillForXP || Req.XPGain <= 0.f) return;

	const float StartFrac = (Req.XPOnStartFraction < 0.f) ? DefaultXPOnStartFraction : Req.XPOnStartFraction;
	const float Remainder = FMath::Max(0.f, 1.f - FMath::Clamp(StartFrac, 0.f, 1.f));
	const float Amount = Remainder * Req.XPGain;
	if (Amount <= 0.f) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	if (!Recipient) return;

	// TODO: Apply AddXPEffectClass to Recipient’s ASC for Amount
}

bool UCraftingStationComponent::IsPresenceSatisfied(AActor* Instigator, const FCraftingRequest& Req) const
{
	if (Req.PresencePolicy == ECraftPresencePolicy::None || !Instigator) return true;

	const AActor* Owner = GetOwner();
	if (!Owner) return true;

	const float DistSq = FVector::DistSquared(Owner->GetActorLocation(), Instigator->GetActorLocation());
	return DistSq <= FMath::Square(Req.PresenceRadius);
}

void UCraftingStationComponent::GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out)
{
	if (!Viewer) return;

	// 1) ASC on the actor
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Viewer))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->GetOwnedGameplayTags(Out);
		}
	}

	// 2) Actor implements tag interface
	if (IGameplayTagAssetInterface* TagIF = Cast<IGameplayTagAssetInterface>(Viewer))
	{
		TagIF->GetOwnedGameplayTags(Out);
	}

	// 3) Pawn -> PlayerState
	if (const APawn* Pawn = Cast<APawn>(Viewer))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (const IAbilitySystemInterface* ASI2 = Cast<IAbilitySystemInterface>(PS))
			{
				if (UAbilitySystemComponent* ASC2 = ASI2->GetAbilitySystemComponent())
				{
					ASC2->GetOwnedGameplayTags(Out);
				}
			}
			if (IGameplayTagAssetInterface* TagIF2 = Cast<IGameplayTagAssetInterface>(PS))
			{
				TagIF2->GetOwnedGameplayTags(Out);
			}
		}
	}

	// 4) Controller -> PlayerState
	if (const AController* PC = Cast<AController>(Viewer))
	{
		if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
		{
			if (const IAbilitySystemInterface* ASI3 = Cast<IAbilitySystemInterface>(PS))
			{
				if (UAbilitySystemComponent* ASC3 = ASI3->GetAbilitySystemComponent())
				{
					ASC3->GetOwnedGameplayTags(Out);
				}
			}
			if (IGameplayTagAssetInterface* TagIF3 = Cast<IGameplayTagAssetInterface>(PS))
			{
				TagIF3->GetOwnedGameplayTags(Out);
			}
		}
	}
}
