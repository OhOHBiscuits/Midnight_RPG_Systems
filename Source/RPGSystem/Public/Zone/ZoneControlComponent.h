#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ZoneControlComponent.generated.h"

class USmartZoneComponent;

UENUM(BlueprintType)
enum class EZoneControlState : uint8
{
	Neutral     UMETA(DisplayName="Neutral"),
	Contested   UMETA(DisplayName="Contested"),
	Controlled  UMETA(DisplayName="Controlled")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneControlChanged, EZoneControlState, NewState, FGameplayTag, ControlledByFaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneCaptureProgress, float, Progress01, FGameplayTag, AttackerFaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneCombatChanged, bool, bInCombat);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class RPGSYSTEM_API UZoneControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZoneControlComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Control")
	FGameplayTag InitialOwnerFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Control")
	FGameplayTag PlayerFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Control")
	FGameplayTag AttackerFaction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Tags")
	FGameplayTagContainer CombatZoneTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Tags")
	FGameplayTagContainer SafeZoneTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Tags")
	FGameplayTagContainer OutpostZoneTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Capture")
	float CaptureRatePerAttacker = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Capture")
	float DefenderWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Capture")
	float DecayRatePerSecond = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Capture")
	float CaptureTickInterval = 0.25f;

	UPROPERTY(ReplicatedUsing=OnRep_ControlState, VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|State")
	EZoneControlState ControlState = EZoneControlState::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_OwnerFaction, VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|State")
	FGameplayTag CurrentOwnerFaction;

	UPROPERTY(ReplicatedUsing=OnRep_AttackerFaction, VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|State")
	FGameplayTag CurrentAttackerFaction;

	UPROPERTY(ReplicatedUsing=OnRep_CaptureProgress, VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|State")
	float CaptureProgress01 = 0.f;

	UPROPERTY(ReplicatedUsing=OnRep_Combat, VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|State")
	bool bInCombat = false;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneControlChanged OnControlChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneCaptureProgress OnCaptureProgress;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneCombatChanged OnCombatChanged;

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void TryForceLiberateToPlayer();

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void TryForceOwnerFaction(FGameplayTag NewOwner);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void TryForceCombat(bool bCombat);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void TryResetZone();

	UFUNCTION(BlueprintCallable, Category="1_Zones|Presence")
	void NotifyActorEntered(AActor* Other);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Presence")
	void NotifyActorLeft(AActor* Other);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_ControlState();
	UFUNCTION() void OnRep_OwnerFaction();
	UFUNCTION() void OnRep_AttackerFaction();
	UFUNCTION() void OnRep_CaptureProgress();
	UFUNCTION() void OnRep_Combat();

	void ServerSetOwner(FGameplayTag NewOwner);
	void ServerSetCombat(bool bCombat);
	void ServerReset();

	void StartCaptureTimer();
	void StopCaptureTimerIfIdle();
	void TickCapture();

	bool IsActorFaction(AActor* Actor, const FGameplayTag& Faction) const;
	class APlayerState* ResolvePS(AActor* Actor) const;
	void PushModeToSmartZone();

	UFUNCTION(Server, Reliable, WithValidation) void Server_ForceLiberateToPlayer();
	UFUNCTION(Server, Reliable, WithValidation) void Server_ForceOwnerFaction(FGameplayTag NewOwner);
	UFUNCTION(Server, Reliable, WithValidation) void Server_ForceCombat(bool bCombat);
	UFUNCTION(Server, Reliable, WithValidation) void Server_ResetZone();

private:
	TWeakObjectPtr<USmartZoneComponent> SmartZone;

	FTimerHandle CaptureTimer;

	int32 NumAttackers = 0;
	int32 NumDefenders = 0;
	UPROPERTY() TSet<TWeakObjectPtr<class APlayerState>> PresentPlayers;
};
