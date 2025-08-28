#include "Crafting/CraftingStationComponent.h"
#include "Crafting/CraftingRecipeDataAsset.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"

#include "Progression/SkillCheckBlueprintLibrary.h"
#include "Progression/ProgressionBlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

#include "TimerManager.h"
#include "Engine/World.h"

// If you plan to replicate this component or its state later:
// #include "Net/UnrealNetwork.h"

UCraftingStationComponent::UCraftingStationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	bOutputToStationInventory = true;
	DefaultXPOnStartFraction = 0.0f;
}

void UCraftingStationComponent::BeginPlay()
{
	Super::BeginPlay();
}

static UInventoryComponent* GetInvFromActor(AActor* Actor)
{
	return Actor ? UInventoryHelpers::GetInventoryComponent(Actor) : nullptr;
}

UInventoryComponent* UCraftingStationComponent::ResolveOutputInventory(AActor* Instigator) const
{
	if (OutputInventoryOverride) return OutputInventoryOverride;
	if (bOutputToStationInventory) return GetInvFromActor(GetOwner());
	return GetInvFromActor(Instigator);
}

void UCraftingStationComponent::GrantOutputs(const TArray<FCraftItemOutput>& Outputs, int32 QualityTier, AActor* Instigator)
{
	if (UInventoryComponent* Inv = ResolveOutputInventory(Instigator))
	{
		for (const FCraftItemOutput& O : Outputs)
		{
			if (!O.Item) continue;

			int32 Qty = O.Quantity;
			if (QualityTier > 0)
			{
				Qty += FMath::FloorToInt(QualityTier * 0.1f * O.Quantity);
			}
			Inv->AddItem(O.Item, FMath::Max(1, Qty));
		}
	}
}

void UCraftingStationComponent::GiveStartXPIfAny(const FCraftingRequest& Req, AActor* Instigator)
{
	if (!AddXPEffectClass || !Req.SkillForXP || Req.XPGain <= 0.f) return;

	const float Split = (Req.XPOnStartFraction >= 0.f) ? FMath::Clamp(Req.XPOnStartFraction, 0.f, 1.f) : DefaultXPOnStartFraction;
	const float XPAmt = Req.XPGain * Split;
	if (XPAmt <= 0.f) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	if (!Recipient) return;

	UProgressionBlueprintLibrary::ApplySkillXP_GE(Recipient, Recipient, AddXPEffectClass, Req.SkillForXP, XPAmt, -1.f, true);
}

void UCraftingStationComponent::GiveFinishXPIfAny(const FCraftingRequest& Req, AActor* Instigator, bool bSuccess)
{
	if (!bSuccess) return;
	if (!AddXPEffectClass || !Req.SkillForXP || Req.XPGain <= 0.f) return;

	const float StartSplit = (Req.XPOnStartFraction >= 0.f) ? FMath::Clamp(Req.XPOnStartFraction, 0.f, 1.f) : DefaultXPOnStartFraction;
	const float FinishSplit = 1.f - StartSplit;
	const float XPAmt = Req.XPGain * FinishSplit;
	if (XPAmt <= 0.f) return;

	AActor* Recipient = Req.XPRecipient.IsValid() ? Req.XPRecipient.Get() : Instigator;
	if (!Recipient) return;

	UProgressionBlueprintLibrary::ApplySkillXP_GE(Recipient, Recipient, AddXPEffectClass, Req.SkillForXP, XPAmt, -1.f, true);
}

bool UCraftingStationComponent::IsPresenceSatisfied(AActor* Instigator, const FCraftingRequest& Req) const
{
	if (!Instigator) return false;
	if (Req.PresencePolicy == ECraftPresencePolicy::None) return true;

	const AActor* StationOwner = GetOwner();
	if (!StationOwner) return false;

	const float Dist = FVector::Dist(StationOwner->GetActorLocation(), Instigator->GetActorLocation());
	return Dist <= FMath::Max(0.f, Req.PresenceRadius);
}

bool UCraftingStationComponent::StartCraft(AActor* Instigator, const FCraftingRequest& Request)
{
	if (!GetOwner()) return false;

	// Client → Server
	if (!GetOwner()->HasAuthority())
	{
		ServerStartCraft(Instigator, Request);
		return true; // request sent
	}

	// Server only
	if (bIsCrafting) return false;

	FCraftingRequest Req = Request;
	if (!Req.CheckDef) Req.CheckDef = DefaultCheck;
	if (!Req.CheckDef) return false;

	// Presence gate at start if required
	if (Req.PresencePolicy == ECraftPresencePolicy::RequireAtStart ||
		Req.PresencePolicy == ECraftPresencePolicy::RequireStartFinish)
	{
		if (!IsPresenceSatisfied(Instigator, Req))
		{
			return false;
		}
	}

	// Skill check
	FSkillCheckParams P;
	FSkillCheckResult R;
	if (!USkillCheckBlueprintLibrary::ComputeSkillCheck(Instigator ? Instigator : GetOwner(), GetOwner(), Req.CheckDef, P, R))
	{
		return false;
	}

	const float FinalTime = FMath::Max(0.0f, Req.BaseTimeSeconds) * FMath::Max(0.01f, R.TimeMultiplier);

	ActiveRequest = Req;
	ActiveCheck = R;
	CraftInstigator = Instigator;
	bIsCrafting = true;

	GiveStartXPIfAny(ActiveRequest, Instigator);
	OnCraftStarted.Broadcast(ActiveRequest, FinalTime, ActiveCheck);

	GetWorld()->GetTimerManager().SetTimer(CraftTimer, this, &UCraftingStationComponent::FinishCraft, FinalTime, false);
	return true;
}

// -------- Server RPC body (MUST be this exact name/signature) --------
void UCraftingStationComponent::ServerStartCraft_Implementation(AActor* Instigator, FCraftingRequest Request)
{
	StartCraft(Instigator, Request);
}
// --------------------------------------------------------------------

bool UCraftingStationComponent::StartCraftFromRecipe(AActor* Instigator, UCraftingRecipeDataAsset* Recipe)
{
	if (!Recipe) return false;

	// Reflective ingestion of your existing recipe asset:
	FCraftingRequest Req;

	// Inputs
	if (FArrayProperty* Arr = FindFProperty<FArrayProperty>(Recipe->GetClass(), TEXT("Inputs")))
	{
		if (FStructProperty* Inner = CastField<FStructProperty>(Arr->Inner))
		{
			FScriptArrayHelper Helper(Arr, Arr->ContainerPtrToValuePtr<void>(Recipe));
			for (int32 i=0; i<Helper.Num(); ++i)
			{
				void* Elem = Helper.GetRawPtr(i);

				FGameplayTag Tag;
				if (FStructProperty* PTag = FindFProperty<FStructProperty>(Inner->Struct, TEXT("ItemID")))
				{
					if (PTag->Struct == TBaseStructure<FGameplayTag>::Get())
						Tag = *PTag->ContainerPtrToValuePtr<FGameplayTag>(Elem);
				}
				else if (FStructProperty* PTag2 = FindFProperty<FStructProperty>(Inner->Struct, TEXT("ItemTag")))
				{
					if (PTag2->Struct == TBaseStructure<FGameplayTag>::Get())
						Tag = *PTag2->ContainerPtrToValuePtr<FGameplayTag>(Elem);
				}
				else if (FStructProperty* PTag3 = FindFProperty<FStructProperty>(Inner->Struct, TEXT("ID")))
				{
					if (PTag3->Struct == TBaseStructure<FGameplayTag>::Get())
						Tag = *PTag3->ContainerPtrToValuePtr<FGameplayTag>(Elem);
				}

				int32 Qty = 1;
				if (FIntProperty* PQ = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Quantity")))   Qty = PQ->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ2 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Qty")))  Qty = PQ2->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ3 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Count")))Qty = PQ3->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ4 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Amount")))Qty = PQ4->GetPropertyValue_InContainer(Elem);

				FCraftItemCost Cost;
				Cost.ItemID = Tag;
				Cost.Quantity = FMath::Max(1, Qty);
				Req.Inputs.Add(Cost);
			}
		}
	}

	// Outputs
	if (FArrayProperty* ArrOut = FindFProperty<FArrayProperty>(Recipe->GetClass(), TEXT("Outputs")))
	{
		if (FStructProperty* Inner = CastField<FStructProperty>(ArrOut->Inner))
		{
			FScriptArrayHelper Helper(ArrOut, ArrOut->ContainerPtrToValuePtr<void>(Recipe));
			for (int32 i=0; i<Helper.Num(); ++i)
			{
				void* Elem = Helper.GetRawPtr(i);

				UItemDataAsset* Item = nullptr;
				if (FObjectProperty* PO = FindFProperty<FObjectProperty>(Inner->Struct, TEXT("Item")))
				{
					if (PO->PropertyClass->IsChildOf(UItemDataAsset::StaticClass()))
						Item = *PO->ContainerPtrToValuePtr<UItemDataAsset*>(Elem);
				}
				else if (FObjectProperty* PO2 = FindFProperty<FObjectProperty>(Inner->Struct, TEXT("ItemAsset")))
				{
					if (PO2->PropertyClass->IsChildOf(UItemDataAsset::StaticClass()))
						Item = *PO2->ContainerPtrToValuePtr<UItemDataAsset*>(Elem);
				}
				else if (FObjectProperty* PO3 = FindFProperty<FObjectProperty>(Inner->Struct, TEXT("ItemData")))
				{
					if (PO3->PropertyClass->IsChildOf(UItemDataAsset::StaticClass()))
						Item = *PO3->ContainerPtrToValuePtr<UItemDataAsset*>(Elem);
				}

				int32 Qty = 1;
				if (FIntProperty* PQ = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Quantity")))   Qty = PQ->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ2 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Qty")))  Qty = PQ2->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ3 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Count")))Qty = PQ3->GetPropertyValue_InContainer(Elem);
				else if (FIntProperty* PQ4 = FindFProperty<FIntProperty>(Inner->Struct, TEXT("Amount")))Qty = PQ4->GetPropertyValue_InContainer(Elem);

				if (Item)
				{
					FCraftItemOutput Out;
					Out.Item = Item;
					Out.Quantity = FMath::Max(1, Qty);
					Req.Outputs.Add(Out);
				}
			}
		}
	}

	// Base time (try a few common field names, default 1s)
	Req.BaseTimeSeconds = 1.0f;
	if (FFloatProperty* PT = FindFProperty<FFloatProperty>(Recipe->GetClass(), TEXT("BaseTimeSeconds")))
		Req.BaseTimeSeconds = PT->GetPropertyValue_InContainer(Recipe);
	else if (FFloatProperty* PT2 = FindFProperty<FFloatProperty>(Recipe->GetClass(), TEXT("CraftTime")))
		Req.BaseTimeSeconds = PT2->GetPropertyValue_InContainer(Recipe);
	else if (FFloatProperty* PT3 = FindFProperty<FFloatProperty>(Recipe->GetClass(), TEXT("TimeToCraft")))
		Req.BaseTimeSeconds = PT3->GetPropertyValue_InContainer(Recipe);

	Req.XPRecipient = Instigator;

	return StartCraft(Instigator, Req);
}

void UCraftingStationComponent::CancelCraft()
{
	if (!bIsCrafting) return;
	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);
	bIsCrafting = false;
	ActiveRequest = FCraftingRequest{};
	ActiveCheck = FSkillCheckResult{};
	CraftInstigator = nullptr;
}

void UCraftingStationComponent::FinishCraft()
{
	GetWorld()->GetTimerManager().ClearTimer(CraftTimer);

	bool bSuccess = ActiveCheck.bSuccess;
	AActor* Inst = CraftInstigator.Get();

	if (bSuccess && (ActiveRequest.PresencePolicy == ECraftPresencePolicy::RequireAtFinish ||
					 ActiveRequest.PresencePolicy == ECraftPresencePolicy::RequireStartFinish))
	{
		if (!IsPresenceSatisfied(Inst, ActiveRequest))
		{
			bSuccess = false;
		}
	}

	if (bSuccess)
	{
		GrantOutputs(ActiveRequest.Outputs, ActiveCheck.QualityTier, Inst);
	}

	GiveFinishXPIfAny(ActiveRequest, Inst, bSuccess);
	OnCraftCompleted.Broadcast(ActiveRequest, ActiveCheck, bSuccess);

	bIsCrafting = false;
	ActiveRequest = FCraftingRequest{};
	ActiveCheck = FSkillCheckResult{};
	CraftInstigator = nullptr;
}
