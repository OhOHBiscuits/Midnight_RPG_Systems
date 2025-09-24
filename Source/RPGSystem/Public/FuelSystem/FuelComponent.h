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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Fuel|Inventories")
	UInventoryComponent* FuelInventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Fuel|Inventories")
	UInventoryComponent* ByproductInventory = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Fuel|State")
	float TotalBurnTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Fuel|State")
	float RemainingBurnTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Fuel|Tuning")
	float BurnSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Fuel|State")
	float LastBurnTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Fuel|Behavior")
	bool bAutoStopBurnWhenIdle = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Inventory-Fuel|State")
	bool bIsBurning = false;

	/** Resolve byproduct items via tag -> data asset at burn time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Inventory-Fuel|Tags")
	bool bResolveByproductByTag = true;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Fuel|Events")
	FFuelProgressEvent OnFuelBurnProgress;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Fuel|Events")
	FFuelDepletedEvent OnFuelDepleted;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Fuel|Events")
	FOnBurnStarted OnBurnStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Inventory-Fuel|Events")
	FOnBurnStopped OnBurnStopped;

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Actions")
	virtual void StartBurn();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Actions")
	virtual void StopBurn();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Actions")
	virtual void PauseBurn();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Actions")
	virtual void ResumeBurn();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory-Fuel|Queries")
	bool HasFuel() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Inventory-Fuel|Queries")
	bool IsBurning() const { return bIsBurning; }

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Actions")
	void TryStartNextFuel();

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Behavior")
	virtual bool ShouldKeepBurning() const;

	UFUNCTION(BlueprintCallable, Category="1_Inventory-Fuel|Hooks")
	virtual void OnCraftingActivated();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FTimerHandle BurnFuelTimer;

	UFUNCTION()
	virtual void BurnTimerTick();

	void BurnFuelOnce();
	void NotifyFuelStateChanged();

	// Allow stations to decide if they are "active"
	virtual bool IsCraftingActive() const { return true; }

private:
	bool HasAuth() const
	{
		const AActor* Owner = GetOwner();
		return Owner && Owner->HasAuthority();
	}

	void DoAutoStop();
};
