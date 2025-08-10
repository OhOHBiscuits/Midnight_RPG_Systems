#include "DecaySystem/DecayComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryHelpers.h"
#include "Inventory/ItemDataAsset.h"
#include "Inventory/InventoryItem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UDecayComponent::UDecayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDecayComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthoritySafe())
	{
		ResolveInventory();
		ApplySettingsFromInventoryType();
		BindInventoryChanged(true);
		RefreshDecaySlots();
		TryStartStopFromCurrentState();
	}
}

void UDecayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthoritySafe())
	{
		BindInventoryChanged(false);
		StopTimer();
	}
	Super::EndPlay(EndPlayReason);
}

bool UDecayComponent::HasAuthoritySafe() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UDecayComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDecayComponent, bDecayActive);
	DOREPLIFETIME(UDecayComponent, DecayCheckInterval);
	DOREPLIFETIME(UDecayComponent, DecaySpeedMultiplier);
	DOREPLIFETIME(UDecayComponent, EfficiencyRating);
	DOREPLIFETIME(UDecayComponent, InputBatchSize);
	DOREPLIFETIME(UDecayComponent, DecaySlots);
	DOREPLIFETIME(UDecayComponent, bIsTrackingAny);
}

// --- API ---
void UDecayComponent::StartDecay()
{
	if (HasAuthoritySafe())
	{
		bDecayActive = true;
		TryStartStopFromCurrentState();
		return;
	}
	Server_StartDecay();
}

void UDecayComponent::StopDecay()
{
	if (HasAuthoritySafe())
	{
		bDecayActive = false;
		TryStartStopFromCurrentState();
		return;
	}
	Server_StopDecay();
}

void UDecayComponent::ForceRefresh()
{
	if (!HasAuthoritySafe()) return;

	ResolveInventory();
	ApplySettingsFromInventoryType();
	const int32 Tracked = RefreshDecaySlots();
	if (bLogDecayDebug) UE_LOG(LogTemp, Log, TEXT("[Decay] ForceRefresh -> %d tracked"), Tracked);
	TryStartStopFromCurrentState();
}

void UDecayComponent::RebindToInventory(UInventoryComponent* InInventory)
{
	if (!HasAuthoritySafe()) return;

	BindInventoryChanged(false);
	Inventory = InInventory;
	BindInventoryChanged(true);

	ApplySettingsFromInventoryType();
	RefreshDecaySlots();
	TryStartStopFromCurrentState();
}

void UDecayComponent::SetInventoryByActor(AActor* InActor)
{
	if (!HasAuthoritySafe() || !InActor) return;

	BindInventoryChanged(false);
	SourceActor = InActor;
	Inventory = InActor->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		Inventory = UInventoryHelpers::GetInventoryComponent(InActor);
	}
	BindInventoryChanged(true);

	ApplySettingsFromInventoryType();
	RefreshDecaySlots();
	TryStartStopFromCurrentState();
}

bool UDecayComponent::GetSlotProgress(int32 SlotIndex, float& OutRemaining, float& OutTotal) const
{
	for (const FDecaySlot& S : DecaySlots)
	{
		if (S.SlotIndex == SlotIndex)
		{
			OutRemaining = S.DecayTimeRemaining;
			OutTotal     = S.TotalDecayTime;
			return true;
		}
	}
	OutRemaining = OutTotal = -1.f;
	return false;
}

bool UDecayComponent::IsIdle() const
{
	if (!Inventory || DecaySlots.Num() == 0) return true;

	const float Speed = FMath::Max(0.f, DecaySpeedMultiplier);
	if (Speed <= 0.f) return true;

	for (const FDecaySlot& S : DecaySlots)
	{
		if (S.SlotIndex < 0) continue;
		if (S.DecayTimeRemaining <= 0.f) continue;

		const FInventoryItem Curr = Inventory->GetItem(S.SlotIndex);
		if (Curr.Quantity >= S.BatchSize)
		{
			return false;
		}
	}
	return true;
}

void UDecayComponent::StopIfIdle()
{
	if (bAutoStopWhenIdle && IsIdle())
	{
		StopTimer();
		bIsTrackingAny = false;
		OnTrackingStateChanged.Broadcast(false);
	}
}

// --- Core ---
void UDecayComponent::ResolveInventory()
{
	if (Inventory) return;

	AActor* Search = SourceActor ? SourceActor : GetOwner();
	if (Search)
	{
		Inventory = Search->FindComponentByClass<UInventoryComponent>();
		if (!Inventory)
		{
			Inventory = UInventoryHelpers::GetInventoryComponent(Search);
		}
	}
}

void UDecayComponent::BindInventoryChanged(bool bBind)
{
	if (!HasAuthoritySafe() || !Inventory) return;

	if (bBind)
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &UDecayComponent::OnInventoryChangedRefresh);
	}
	else
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UDecayComponent::OnInventoryChangedRefresh);
	}
}

void UDecayComponent::ApplySettingsFromInventoryType()
{
	if (!Inventory) return;

	const FGameplayTag T_PlayerBackpack = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.PlayerBackpack"), false);
	const FGameplayTag T_StorageChest   = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.StorageChest"),  false);
	const FGameplayTag T_CoolStorage    = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.CoolStorage"),   false);
	const FGameplayTag T_ColdStorage    = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.ColdStorage"),   false);
	const FGameplayTag T_StoreHouse     = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.StoreHouse"),    false);
	const FGameplayTag T_Compost        = FGameplayTag::RequestGameplayTag(FName("Inventory.Type.Compost"),       false);

	const FGameplayTag& Type = Inventory->InventoryTypeTag;

	if (T_PlayerBackpack.IsValid() && Type.MatchesTagExact(T_PlayerBackpack))       { DecaySpeedMultiplier = 3.0f; EfficiencyRating = 0.5f;  }
	else if (T_StorageChest.IsValid() && Type.MatchesTagExact(T_StorageChest))      { DecaySpeedMultiplier = 1.5f; EfficiencyRating = 1.5f;  }
	else if (T_CoolStorage.IsValid() && Type.MatchesTagExact(T_CoolStorage))        { DecaySpeedMultiplier = 0.5f; EfficiencyRating = 2.0f;  }
	else if (T_ColdStorage.IsValid() && Type.MatchesTagExact(T_ColdStorage))        { DecaySpeedMultiplier = 0.0f; EfficiencyRating = 2.5f;  }
	else if (T_StoreHouse.IsValid() && Type.MatchesTagExact(T_StoreHouse))          { DecaySpeedMultiplier = 1.0f; EfficiencyRating = 1.75f; }
	else if (T_Compost.IsValid() && Type.MatchesTagExact(T_Compost))                { DecaySpeedMultiplier = 5.0f; EfficiencyRating = 3.0f;  }
}

static FORCEINLINE bool SoftValid(const TSoftObjectPtr<UItemDataAsset>& S)
{
	return S.ToSoftObjectPath().IsValid();
}

UItemDataAsset* UDecayComponent::ResolveItemAsset(const FInventoryItem& SlotItem)
{
	if (SlotItem.ItemData.IsValid()) return SlotItem.ItemData.Get();
	if (SlotItem.ItemData.ToSoftObjectPath().IsValid()) return SlotItem.ItemData.LoadSynchronous();
	return nullptr;
}

UItemDataAsset* UDecayComponent::ResolveSoftItem(UItemDataAsset* MaybeLoaded, const TSoftObjectPtr<UItemDataAsset>& Soft)
{
	if (MaybeLoaded) return MaybeLoaded;
	if (SoftValid(Soft)) return Soft.LoadSynchronous();
	return nullptr;
}

float UDecayComponent::GetItemDecaySeconds(const UItemDataAsset* Asset) const
{
	// Assumes your ItemDataAsset has GetTotalDecaySeconds().
	// If not, wire it to whatever fields you use for decay duration.
	return Asset ? Asset->GetTotalDecaySeconds() : 0.f;
}

int32 UDecayComponent::CalculateBatchOutput(int32 InputUsed) const
{
	if (InputUsed <= 0) return 0;
	return FMath::Max(1, FMath::FloorToInt(InputUsed * FMath::Max(0.f, EfficiencyRating)));
}

int32 UDecayComponent::RefreshDecaySlots()
{
	if (!HasAuthoritySafe()) return DecaySlots.Num();

	DecaySlots.Empty();

	if (!Inventory)
	{
		if (bLogDecayDebug) UE_LOG(LogTemp, Log, TEXT("[Decay] Refresh: Inventory=NULL"));
		bIsTrackingAny = false;
		OnTrackingStateChanged.Broadcast(false);
		return 0;
	}

	const TArray<FInventoryItem>& Items = Inventory->GetItems();
	const int32 Batch = FMath::Max(1, InputBatchSize);

	for (int32 i=0; i<Items.Num(); ++i)
	{
		const FInventoryItem& Slot = Items[i];
		if (Slot.Quantity < Batch) continue;

		UItemDataAsset* Asset = ResolveItemAsset(Slot);
		if (!Asset || !Asset->bCanDecay) continue;

		const float Total = GetItemDecaySeconds(Asset);
		if (Total <= 0.f) continue;

		DecaySlots.Emplace(i, Total, Total, Batch);
	}

	const bool bNowTracking = DecaySlots.Num() > 0;
	if (bNowTracking != bIsTrackingAny)
	{
		bIsTrackingAny = bNowTracking;
		OnTrackingStateChanged.Broadcast(bIsTrackingAny);
	}

	if (bLogDecayDebug) UE_LOG(LogTemp, Log, TEXT("[Decay] Refresh -> %d tracked"), DecaySlots.Num());
	return DecaySlots.Num();
}

void UDecayComponent::TryStartStopFromCurrentState()
{
	if (!HasAuthoritySafe() || !IsInventoryValid())
	{
		StopTimer();
		return;
	}

	if (bDecayActive && DecaySlots.Num() > 0 && !IsIdle())
	{
		StartTimer();
	}
	else
	{
		StopTimer();
	}
}

void UDecayComponent::StartTimer()
{
	if (!GetWorld()) return;
	if (!GetWorld()->GetTimerManager().IsTimerActive(DecayTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			DecayTimerHandle, this, &UDecayComponent::DecayTimerTick,
			FMath::Max(0.05f, DecayCheckInterval), true
		);
	}
}

void UDecayComponent::StopTimer()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(DecayTimerHandle);
}

bool UDecayComponent::ConsumeInputAtSlot_Server(int32 SlotIndex, int32 Quantity)
{
	if (!IsInventoryValid() || Quantity <= 0) return false;
	return Inventory->RemoveItem(SlotIndex, Quantity); // direct, server-only
}

void UDecayComponent::CompactTrackedSlots()
{
	if (!HasAuthoritySafe()) return;

	DecaySlots.RemoveAll([this](const FDecaySlot& S)
	{
		if (!Inventory) return true;
		if (S.SlotIndex < 0 || S.DecayTimeRemaining <= 0.f) return true;

		const FInventoryItem Curr = Inventory->GetItem(S.SlotIndex);
		if (Curr.Quantity < S.BatchSize) return true;

		UItemDataAsset* Asset = ResolveItemAsset(Curr);
		if (!Asset || !Asset->bCanDecay) return true;

		return false;
	});

	const bool bNowTracking = DecaySlots.Num() > 0;
	if (bNowTracking != bIsTrackingAny)
	{
		bIsTrackingAny = bNowTracking;
		OnTrackingStateChanged.Broadcast(bIsTrackingAny);
	}
}

// --- Tick ---
void UDecayComponent::DecayTimerTick()
{
	if (!HasAuthoritySafe() || !IsInventoryValid())
	{
		StopTimer();
		return;
	}

	if (!bDecayActive || DecaySlots.Num() == 0)
	{
		StopTimer();
		return;
	}

	bTickInProgress = true;

	bool bAnyCountingDown = false;
	const float Speed = FMath::Max(0.f, DecaySpeedMultiplier);

	for (int32 idx = 0; idx < DecaySlots.Num(); ++idx)
	{
		FDecaySlot& S = DecaySlots[idx];
		if (S.SlotIndex < 0) continue;

		const FInventoryItem Curr = Inventory->GetItem(S.SlotIndex);
		if (Curr.Quantity < S.BatchSize) { S.DecayTimeRemaining = -1.f; continue; }

		UItemDataAsset* Asset = ResolveItemAsset(Curr);
		if (!Asset || !Asset->bCanDecay) { S.DecayTimeRemaining = -1.f; continue; }

		if (Speed <= 0.f)
		{
			OnDecayProgress.Broadcast(S.SlotIndex, S.DecayTimeRemaining, S.TotalDecayTime);
			continue;
		}

		S.DecayTimeRemaining -= (DecayCheckInterval * Speed);
		OnDecayProgress.Broadcast(S.SlotIndex, S.DecayTimeRemaining, S.TotalDecayTime);

		if (S.DecayTimeRemaining > 0.f)
		{
			bAnyCountingDown = true;
			continue;
		}

		// batch completes
		const int32 InputUsed = S.BatchSize;

		UItemDataAsset* OutAsset = ResolveSoftItem(nullptr, Asset->DecaysInto);
		if (OutAsset)
		{
			const int32 OutQty = CalculateBatchOutput(InputUsed);
			Inventory->TryAddItem(OutAsset, OutQty);
			OnItemDecayed.Broadcast(S.SlotIndex, OutAsset, OutQty);
		}

		ConsumeInputAtSlot_Server(S.SlotIndex, InputUsed);

		const FInventoryItem After = Inventory->GetItem(S.SlotIndex);
		if (After.Quantity >= S.BatchSize && Asset->bCanDecay)
		{
			const float Total = GetItemDecaySeconds(Asset);
			S.TotalDecayTime = Total;
			S.DecayTimeRemaining = Total;
			bAnyCountingDown = true;
		}
		else
		{
			S.DecayTimeRemaining = -1.f;
		}
	}

	bTickInProgress = false;

	CompactTrackedSlots();

	if (bRefreshRequested)
	{
		bRefreshRequested = false;
		RefreshDecaySlots();
	}

	if (bAutoStopWhenIdle && !bAnyCountingDown)
	{
		StopIfIdle();
		return;
	}

	if (!bIsTrackingAny || DecaySlots.Num() == 0)
	{
		StopTimer();
	}
}

// --- Inventory change hook ---
void UDecayComponent::OnInventoryChangedRefresh()
{
	if (!HasAuthoritySafe()) return;

	if (bTickInProgress)
	{
		bRefreshRequested = true;
		return;
	}

	RefreshDecaySlots();
	TryStartStopFromCurrentState();
}

// --- RepNotifies ---
void UDecayComponent::OnRep_DecaySlots()
{
	for (const FDecaySlot& S : DecaySlots)
	{
		if (S.SlotIndex >= 0 && S.TotalDecayTime > 0.f)
		{
			OnDecayProgress.Broadcast(S.SlotIndex, S.DecayTimeRemaining, S.TotalDecayTime);
		}
	}
}

void UDecayComponent::OnRep_IsTrackingAny()
{
	OnTrackingStateChanged.Broadcast(bIsTrackingAny);
}

// --- RPCs ---
void UDecayComponent::Server_StartDecay_Implementation()
{
	bDecayActive = true;
	TryStartStopFromCurrentState();
}
bool UDecayComponent::Server_StartDecay_Validate() { return true; }

void UDecayComponent::Server_StopDecay_Implementation()
{
	bDecayActive = false;
	TryStartStopFromCurrentState();
}
bool UDecayComponent::Server_StopDecay_Validate() { return true; }
