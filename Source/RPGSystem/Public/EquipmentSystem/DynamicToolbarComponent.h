// DynamicToolbarComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "DynamicToolbarComponent.generated.h"

class UInventoryComponent;
class UEquipmentComponent;
class UItemDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToolbarChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolbarVisibilityChanged, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveToolChanged, int32, NewIndex, UItemDataAsset*, ItemData);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UDynamicToolbarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDynamicToolbarComponent();

	// Config
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	int32 NumSlots = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	FGameplayTag BaseZoneTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	TMap<FGameplayTag, FGameplayTagQuery> ZoneAllowedQueries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	FGameplayTagQuery CombatAllowedQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	bool bHideInBaseZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	bool bAutoEquipActiveTool = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Toolbar|Config")
	FGameplayTag ActiveToolSlotTag;

	// State
	UPROPERTY(ReplicatedUsing=OnRep_Toolbar, VisibleAnywhere, BlueprintReadOnly, Category="1_Toolbar|State")
	TArray<FGameplayTag> ToolbarItemIDs;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveIndex, VisibleAnywhere, BlueprintReadOnly, Category="1_Toolbar|State")
	int32 ActiveIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Toolbar|State")
	bool bManualOverride = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Toolbar|State")
	bool bInCombat = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Toolbar|State")
	bool bToolbarVisible = true;

	// Events
	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnToolbarChanged OnToolbarChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnToolbarVisibilityChanged OnToolbarVisibilityChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Toolbar|Events")
	FOnActiveToolChanged OnActiveToolChanged;

	// External signals (server-auth)
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void NotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void SetCombatState(bool bNewInCombat);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void SetManualOverride(bool bEnable);

	// Input helpers
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void CycleNext();

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void CyclePrev();

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void SetActiveToolIndex(int32 NewIndex);

	// Client wrappers (auto RPC)
	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void TryNotifyZoneTagsUpdated(const FGameplayTagContainer& ZoneTags);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void TrySetCombatState(bool bNewInCombat);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Signals")
	void TrySetManualOverride(bool bEnable);

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void TryCycleNext();

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void TryCyclePrev();

	UFUNCTION(BlueprintCallable, Category="1_Toolbar|Input")
	void TrySetActiveToolIndex(int32 NewIndex);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_Toolbar();
	UFUNCTION() void OnRep_ActiveIndex();

	void RebuildForZone();
	void UpdateVisibilityForZone();

	UInventoryComponent* ResolveInventory() const;
	UEquipmentComponent* ResolveEquipment() const;

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void ServerNotifyZoneTagsUpdated(FGameplayTagContainer ZoneTags);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetCombatState(bool bNewInCombat);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetManualOverride(bool bEnable);
	UFUNCTION(Server, Reliable, WithValidation) void ServerCycleNext();
	UFUNCTION(Server, Reliable, WithValidation) void ServerCyclePrev();
	UFUNCTION(Server, Reliable, WithValidation) void ServerSetActiveToolIndex(int32 NewIndex);

private:
	UPROPERTY() FGameplayTagContainer CurrentZoneTags;
};
