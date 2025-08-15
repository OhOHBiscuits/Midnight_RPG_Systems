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

	/** Inventories this fuel system uses (same-owner components). Not replicated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	UInventoryComponent* FuelInventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	UInventoryComponent* ByproductInventory = nullptr;

	/** Total time for the currently burning item (replicated). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Fuel")
	float TotalBurnTime = 0.0f;

	/** Remaining time for the currently burning item (replicated + repnotify). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FuelState, Category="Fuel")
	float RemainingBurnTime = 0.0f;

	/** Scales burn rate (>1 = faster). Server-authoritative. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	float BurnSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fuel")
	float LastBurnTime = 0.0f;

	/** If true, will auto-stop when no crafting/load is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuel")
	bool bAutoStopBurnWhenIdle = true;

	/** Whether we are currently burning (replicated + repnotify). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_FuelState, Category="Fuel")
	bool bIsBurning = false;

	/** UI hooks (these will also fire on clients via the OnRep handler). */
	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FFuelProgressEvent OnFuelBurnProgress;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FFuelDepletedEvent OnFuelDepleted;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FOnBurnStarted OnBurnStarted;

	UPROPERTY(BlueprintAssignable, Category="Fuel")
	FOnBurnStopped OnBurnStopped;

	/** One-call public API: call from server or client, it will Just Work™ */
	UFUNCTION(BlueprintCallable, Category="Fuel")
	void StartBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void StopBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void PauseBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void ResumeBurn();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void TryStartNextFuel();   // server does the work

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Fuel")
	bool HasFuel() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Fuel")
	bool ShouldKeepBurning() const;

	UFUNCTION(BlueprintCallable, Category="Fuel")
	virtual void OnCraftingActivated();

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void SetFuelInventory(UInventoryComponent* InInv) { FuelInventory = InInv; }

	UFUNCTION(BlueprintCallable, Category="Fuel")
	void SetByproductInventory(UInventoryComponent* InInv) { ByproductInventory = InInv; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** When either RemainingBurnTime or bIsBurning changes on clients. */
	UFUNCTION()
	void OnRep_FuelState();

	FTimerHandle BurnFuelTimer;

	/** Server-only timer tick */
	UFUNCTION()
	void BurnTimerTick();

	/** Server-only helpers */
	void BurnFuelOnce();
	void NotifyFuelStateChanged();

	/** Server-only: called by StartBurn() when HasAuthority() */
	void StartBurn_ServerImpl();
	void StopBurn_ServerImpl();
	void PauseBurn_ServerImpl();
	void ResumeBurn_ServerImpl();

	/** Virtual hook for workstations that depend on a crafting loop */
	virtual bool IsCraftingActive() const { return true; }

private:
	/** Simple auth guard */
	bool HasAuth() const
	{
		const AActor* Owner = GetOwner();
		return Owner && Owner->HasAuthority();
	}

	/** Client->Server RPCs so you can call the public API from clients */
	UFUNCTION(Server, Reliable) void Server_StartBurn();
	UFUNCTION(Server, Reliable) void Server_StopBurn();
	UFUNCTION(Server, Reliable) void Server_PauseBurn();
	UFUNCTION(Server, Reliable) void Server_ResumeBurn();
};
