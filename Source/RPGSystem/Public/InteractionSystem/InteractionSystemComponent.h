#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionSystemComponent.generated.h"

UENUM(BlueprintType)
enum class EInteractionTraceMode : uint8
{
	Line  UMETA(DisplayName="Line"),
	Sphere UMETA(DisplayName="Sphere"),
	Box UMETA(DisplayName="Box")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnInteractionTargetChanged, AActor*, NewTarget, AActor*, OldTarget);

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UInteractionSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionSystemComponent();

	// Auto start
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scan")
	bool bAutoStartScan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scan", meta=(ClampMin="0.01", UIMin="0.01"))
	float ScanInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace")
	EInteractionTraceMode TraceMode = EInteractionTraceMode::Line;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace", meta=(ClampMin="0.0"))
	float MaxDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace|Sphere", meta=(ClampMin="0.0"))
	float SphereRadius = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace|Box")
	FVector BoxHalfExtent = FVector(30.f);

	/** Optional camera override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scan")
	TObjectPtr<USceneComponent> ViewOverride = nullptr;

	/** Extra actors to ignore (BP-friendly type) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trace", AdvancedDisplay)
	TArray<TObjectPtr<AActor>> AdditionalActorsToIgnore;

	UPROPERTY(BlueprintAssignable, Category="Scan")
	FOnInteractionTargetChanged OnTargetChanged;

	UFUNCTION(BlueprintCallable, Category="Scan")
	void StartScanning();

	UFUNCTION(BlueprintCallable, Category="Scan")
	void StopScanning();

	UFUNCTION(BlueprintCallable, Category="Scan")
	void ScanOnce();

	UFUNCTION(BlueprintPure, Category="Scan")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="Interaction|State")
	AActor* GetTargetActor() const { return CurrentTargetActor; }

	/** Copy out the latest hit result used to select the target. */
	UFUNCTION(BlueprintPure, Category="Interaction|State")
	void GetTargetHitResult(FHitResult& OutHit) const { OutHit = CurrentHit; }

	/** Convenience: run one scan right now (useful after setting params in BP). */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void ForceScan();

	

	UFUNCTION(BlueprintCallable, Category="Trace|Ignore List")
	void AddActorToIgnore(AActor* Actor);
	UFUNCTION(BlueprintCallable, Category="Trace|Ignore List")
	void RemoveActorToIgnore(AActor* Actor);
	UFUNCTION(BlueprintCallable, Category="Trace|Ignore List")
	void ClearAdditionalIgnoredActors();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction|State")
	TObjectPtr<AActor> CurrentTargetActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction|State")
	FHitResult CurrentHit;

	/** Helper you call from your scan code after deciding the winner. */
	void SetCurrentTarget(AActor* NewTarget, const FHitResult& NewHit);

private:
	FTimerHandle ScanTimer;
	TWeakObjectPtr<AActor> CurrentTarget;

	void DoScan_Internal();
	void GetViewPoint(FVector& OutLoc, FRotator& OutRot) const;
	void BuildQueryParams(struct FCollisionQueryParams& OutParams) const;
	AActor* ChooseBestTarget(const TArray<FHitResult>& Hits, const FVector& ViewLoc, const FVector& ViewDir) const;
	void SetTarget(AActor* NewTarget);
};
