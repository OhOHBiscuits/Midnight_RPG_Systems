#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Inventory/InventoryComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UCraftingStationComponent::UCraftingStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCraftingStationComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UCraftingStationComponent::StartCraft(AActor* Instigator, const FCraftingRequest& Request)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (bIsCrafting) return false;
	if (!Instigator) Instigator = GetOwner();

	if (!IsPresenceSatisfied(Instigator, Request))
	{
		return false;
	}

	CraftInstigator = Instigator;
	ActiveRequest   = Request;
	bIsCrafting     = true;

	// Stub skill check (hook up your library here if desired)
	if (ActiveRequest.CheckDef)
	{
		ActiveCheck = FSkillCheckResult();
	}

	FCraftingJob Job;
	Job.Request  = ActiveRequest;
	Job.FinalTime = ActiveRequest.BaseTimeSeconds;
	OnCraftStarted.Broadcast(Job);

	GiveStartXPIfAny(ActiveRequest, Instigator);

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			CraftTimer,
			this,
			&UCraftingStationComponent::FinishCraft,
			FMath::Max(ActiveRequest.BaseTimeSeconds, 0.01f),
			false);
	}
	return true;
}

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* Instigator, const UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe) return false;

	FCraftingRequest Req;
	Req.RecipeID         = Recipe->RecipeIDTag;
	Req.BaseTimeSeconds  = Recipe->BaseTimeSeconds;
	Req.CheckDef         = Recipe->CheckDef ? Recipe->CheckDef : DefaultCheck;
	Req.SkillForXP       = Recipe->SkillForXP;
	Req.XPGain           = Recipe->XPGain;
	Req.XPOnStartFraction= (Recipe->XPOnStartFraction >= 0.f) ? Recipe->XPOnStartFraction : DefaultXPOnStartFraction;
	Req.PresencePolicy   = Recipe->PresencePolicy;
	Req.PresenceRadius   = Recipe->PresenceRadius;

	Req.Inputs  = Recipe->Inputs;
	Req.Outputs = Recipe->Outputs;

	return StartCraft(Instigator, Req);
}

void UCraftingStationComponent::CancelCraft()
{
	if (!bIsCrafting) return;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(CraftTimer);
	}

	const bool bSuccess = false;

	FCraftingJob Job;
	Job.Request  = ActiveRequest;
	Job.FinalTime = ActiveRequest.BaseTimeSeconds;

	bIsCrafting   = false;
	ActiveRequest = FCraftingRequest();
	ActiveCheck   = FSkillCheckResult();

	OnCraftFinished.Broadcast(Job, bSuccess);
}

void UCraftingStationComponent::FinishCraft()
{
	const bool bSuccess = true; // Replace with your success logic

	AActor* Instigator = CraftInstigator.Get();
	GrantOutputs(ActiveRequest.Outputs, 0, Instigator);

	GiveFinishXPIfAny(ActiveRequest, Instigator, bSuccess);

	FCraftingJob Job;
	Job.Request  = ActiveRequest;
	Job.FinalTime = ActiveRequest.BaseTimeSeconds;

	bIsCrafting   = false;
	ActiveRequest = FCraftingRequest();
	ActiveCheck   = FSkillCheckResult();

	OnCraftFinished.Broadcast(Job, bSuccess);
}

UInventoryComponent* UCraftingStationComponent::ResolveOutputInventory(AActor* Instigator) const
{
	if (OutputInventoryOverride) return OutputInventoryOverride;

	if (bOutputToStationInventory)
	{
		if (const AActor* Owner = GetOwner())
		{
			return const_cast<AActor*>(Owner)->FindComponentByClass<UInventoryComponent>();
		}
	}

	return Instigator ? Instigator->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UCraftingStationComponent::GrantOutputs(const TArray<FCraftItemOutput>& Outputs, int32 /*QualityTier*/, AActor* Instigator)
{
	if (Outputs.Num() == 0) return;

	if (UInventoryComponent* OutInv = ResolveOutputInventory(Instigator))
	{
		for (const FCraftItemOutput& Out : Outputs)
		{
			if (!Out.Item || Out.Quantity <= 0) continue;

			// TODO: call your actual inventory API here:
			// OutInv->AddItemByAsset(Out.Item, Out.Quantity);

			const TCHAR* ItemName = Out.Item ? *Out.Item->GetName() : TEXT("None");
			const TCHAR* InvName  = OutInv   ? *OutInv->GetName()   : TEXT("None");
			UE_LOG(LogTemp, Log, TEXT("[Crafting] Would grant %d x %s to %s"), Out.Quantity, ItemName, InvName);
		}
	}
	else
	{
		for (const FCraftItemOutput& Out : Outputs)
		{
			const TCHAR* ItemName = Out.Item ? *Out.Item->GetName() : TEXT("None");
			UE_LOG(LogTemp, Warning, TEXT("[Crafting] No inventory to receive %d x %s"), Out.Quantity, ItemName);
		}
	}
}

static UAbilitySystemComponent* GetASCFromAny(const UObject* Obj)
{
	if (!Obj) return nullptr;

	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Obj))
	{
		return ASI->GetAbilitySystemComponent();
	}
	if (const AActor* Actor = Cast<AActor>(Obj))
	{
		return Actor->FindComponentByClass<UAbilitySystemComponent>();
	}
	return nullptr;
}

void UCraftingStationComponent::GiveStartXPIfAny(const FCraftingRequest& Req, AActor* Instigator)
{
	if (Req.XPGain <= 0.f || !Req.SkillForXP) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	const float Fraction = (Req.XPOnStartFraction >= 0.f) ? Req.XPOnStartFraction : DefaultXPOnStartFraction;

	if (Fraction > 0.f && Recipient)
	{
		if (UAbilitySystemComponent* ASC = GetASCFromAny(Recipient))
		{
			// TODO: apply your XP logic here (GE or direct grant)
			UE_LOG(LogTemp, Log, TEXT("[Crafting] Start XP %.2f to %s"), Req.XPGain * Fraction, *Recipient->GetName());
		}
	}
}

void UCraftingStationComponent::GiveFinishXPIfAny(const FCraftingRequest& Req, AActor* Instigator, bool /*bSuccess*/)
{
	if (Req.XPGain <= 0.f || !Req.SkillForXP) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	const float Fraction = (Req.XPOnStartFraction >= 0.f) ? (1.f - Req.XPOnStartFraction) : (1.f - DefaultXPOnStartFraction);

	if (Fraction > 0.f && Recipient)
	{
		if (UAbilitySystemComponent* ASC = GetASCFromAny(Recipient))
		{
			// TODO: apply your XP logic here (GE or direct grant)
			UE_LOG(LogTemp, Log, TEXT("[Crafting] Finish XP %.2f to %s"), Req.XPGain * Fraction, *Recipient->GetName());
		}
	}
}

bool UCraftingStationComponent::IsPresenceSatisfied(AActor* Instigator, const FCraftingRequest& Req) const
{
	if (!Instigator) return false;
	if (Req.PresencePolicy == ECraftPresencePolicy::None) return true;

	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	const FVector InstLoc  = Instigator->GetActorLocation();
	const float   Dist     = FVector::Dist(OwnerLoc, InstLoc);
	return Dist <= Req.PresenceRadius;
}

void UCraftingStationComponent::GatherOwnedTagsFromActor(AActor* Viewer, FGameplayTagContainer& Out)
{
	if (!Viewer) return;

	// Use the C++ interface directly (Execute_ wrapper is NOT provided for this one)
	if (const IGameplayTagAssetInterface* Iface = Cast<IGameplayTagAssetInterface>(Viewer))
	{
		Iface->GetOwnedGameplayTags(Out);
	}

	// Try PlayerState
	if (const APawn* Pawn = Cast<APawn>(Viewer))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (const IGameplayTagAssetInterface* IfacePS = Cast<IGameplayTagAssetInterface>(PS))
			{
				IfacePS->GetOwnedGameplayTags(Out);
			}
		}
	}

	// Try Controller
	if (const APawn* Pawn2 = Cast<APawn>(Viewer))
	{
		if (AController* PC = Pawn2->GetController())
		{
			if (const IGameplayTagAssetInterface* IfacePC = Cast<IGameplayTagAssetInterface>(PC))
			{
				IfacePC->GetOwnedGameplayTags(Out);
			}
		}
	}
}
