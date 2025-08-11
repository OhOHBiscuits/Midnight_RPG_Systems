#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "FuelComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFuelProgressEvent, float, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFuelDepletedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBurnStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBurnStopped);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGSYSTEM_API UFuelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFuelComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	UInventoryComponent* FuelInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	UInventoryComponent* ByproductInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	float TotalBurnTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	float RemainingBurnTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	float LastBurnTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	bool bAutoStopBurnWhenIdle = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	bool bIsBurning = false;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FFuelProgressEvent OnFuelBurnProgress;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FFuelDepletedEvent OnFuelDepleted;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FOnBurnStarted OnBurnStarted;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FOnBurnStopped OnBurnStopped;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void StartBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void StopBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void PauseBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void ResumeBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void TryStartNextFuel();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	bool HasFuel() const;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual bool ShouldKeepBurning() const;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void OnCraftingActivated();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FTimerHandle BurnFuelTimer;

	UFUNCTION()
	virtual void BurnTimerTick();

	void BurnFuelOnce();
	void NotifyFuelStateChanged();

	virtual bool IsCraftingActive() const { return true; }

private:
	/** Simple auth guard */
	bool HasAuth() const
	{
		const AActor* Owner = GetOwner();
		return Owner && Owner->HasAuthority();
	}
};
