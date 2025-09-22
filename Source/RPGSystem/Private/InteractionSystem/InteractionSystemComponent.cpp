#include "InteractionSystem/InteractionSystemComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"

UInteractionSystemComponent::UInteractionSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionSystemComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStartScan)
	{
		StartScanning();
	}
}

void UInteractionSystemComponent::StartScanning()
{
	if (!GetWorld()) return;
	if (!GetWorld()->GetTimerManager().IsTimerActive(ScanTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(
			ScanTimer, this, &UInteractionSystemComponent::DoScan_Internal,
			FMath::Max(0.01f, ScanInterval), true);
	}
}

void UInteractionSystemComponent::StopScanning()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(ScanTimer);
}

void UInteractionSystemComponent::ScanOnce()
{
	DoScan_Internal();
}

void UInteractionSystemComponent::GetViewPoint(FVector& OutLoc, FRotator& OutRot) const
{
	if (ViewOverride)
	{
		OutLoc = ViewOverride->GetComponentLocation();
		OutRot = ViewOverride->GetComponentRotation();
		return;
	}

	// Prefer player camera if owner is possessed
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			PC->GetPlayerViewPoint(OutLoc, OutRot);
			return;
		}
	}

	// Fallback: actor eyes / transform
	if (const AActor* Owner = GetOwner())
	{
		Owner->GetActorEyesViewPoint(OutLoc, OutRot);
	}
}

void UInteractionSystemComponent::BuildQueryParams(FCollisionQueryParams& OutParams) const
{
	OutParams = FCollisionQueryParams(SCENE_QUERY_STAT(InteractionScan), /*bTraceComplex*/ false);
	OutParams.bReturnPhysicalMaterial = false;

	if (AActor* Owner = GetOwner())
	{
		OutParams.AddIgnoredActor(Owner);

		// ignore controller, etc., if present
		if (const APawn* P = Cast<APawn>(Owner))
		{
			if (AController* C = P->GetController())
			{
				if (AActor* AsActor = Cast<AActor>(C))
				{
					OutParams.AddIgnoredActor(AsActor);
				}
			}
		}
	}

	for (AActor* A : AdditionalActorsToIgnore)
	{
		if (IsValid(A)) { OutParams.AddIgnoredActor(A); }
	}
}

AActor* UInteractionSystemComponent::ChooseBestTarget(const TArray<FHitResult>& Hits, const FVector& ViewLoc, const FVector& ViewDir) const
{
	AActor* Best = nullptr;
	float BestDot = -1.f; // prefer most in-front
	for (const FHitResult& H : Hits)
	{
		AActor* A = H.GetActor();
		if (!IsValid(A)) continue;

		const FVector To = (A->GetActorLocation() - ViewLoc).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewDir, To);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = A;
		}
	}
	return Best;
}

void UInteractionSystemComponent::DoScan_Internal()
{
	if (!GetWorld()) return;

	FVector ViewLoc; FRotator ViewRot;
	GetViewPoint(ViewLoc, ViewRot);
	const FVector Dir = ViewRot.Vector();
	const FVector End = ViewLoc + Dir * MaxDistance;

	FCollisionQueryParams Params;
	BuildQueryParams(Params);

	TArray<FHitResult> Hits;

	switch (TraceMode)
	{
	case EInteractionTraceMode::Line:
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, TraceChannel, Params))
		{
			Hits.Add(Hit);
		}
		break;
	}
	case EInteractionTraceMode::Sphere:
	{
		GetWorld()->SweepMultiByChannel(
			Hits, ViewLoc, End, FQuat::Identity, TraceChannel,
			FCollisionShape::MakeSphere(SphereRadius), Params);
		break;
	}
	case EInteractionTraceMode::Box:
	{
		GetWorld()->SweepMultiByChannel(
			Hits, ViewLoc, End, ViewRot.Quaternion(), TraceChannel,
			FCollisionShape::MakeBox(BoxHalfExtent), Params);
		break;
	}
	default: break;
	}

	AActor* Best = Hits.Num() ? ChooseBestTarget(Hits, ViewLoc, Dir) : nullptr;
	SetTarget(Best);
}

void UInteractionSystemComponent::SetTarget(AActor* NewTarget)
{
	AActor* Old = CurrentTarget.Get();
	if (Old == NewTarget) return;

	CurrentTarget = NewTarget;
	OnTargetChanged.Broadcast(NewTarget, Old);
}

void UInteractionSystemComponent::AddActorToIgnore(AActor* Actor)
{
	if (IsValid(Actor))
	{
		AdditionalActorsToIgnore.AddUnique(Actor);
	}
}

void UInteractionSystemComponent::RemoveActorToIgnore(AActor* Actor)
{
	AdditionalActorsToIgnore.Remove(Actor);
}

void UInteractionSystemComponent::ClearAdditionalIgnoredActors()
{
	AdditionalActorsToIgnore.Reset();
}

void UInteractionSystemComponent::ForceScan()
{
	// Call whatever you named your single-tick routine
	// If your function is ScanOnce() just call that:
	// ScanOnce();
	// If it returns a hit/actor, set it via SetCurrentTarget(...)
	// (See below for SetCurrentTarget)
}

// --- NEW ---
void UInteractionSystemComponent::SetCurrentTarget(AActor* NewTarget, const FHitResult& NewHit)
{
	AActor* Old = CurrentTargetActor;
	const bool bChanged = (Old != NewTarget);

	CurrentTargetActor = NewTarget;
	CurrentHit         = NewHit;    // keep this updated even if actor didn’t change

	if (bChanged)
	{
		OnTargetChanged.Broadcast(CurrentTargetActor, Old);
	}
}
