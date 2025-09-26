#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

// Re-broadcast delegate types
#include "Zone/SmartZoneComponent.h"
#include "Zone/ZoneControlComponent.h"

#include "ZoneVolumeActor.generated.h"

class UBoxComponent;

UCLASS()
class RPGSYSTEM_API AZoneVolumeActor : public AActor
{
	GENERATED_BODY()

public:
	AZoneVolumeActor();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|Components")
	UBoxComponent* ZoneBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|Components")
	USmartZoneComponent* Zone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="1_Zones|Components")
	UZoneControlComponent* Control;

	// Quick Config
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config")
	bool bOnlyAffectPlayers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config")
	FGameplayTagContainer DefaultZoneTags;

	// Actor-level Events (rebroadcasted for easy UI hookup)
	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneApplied OnZoneEntered;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneRemoved OnZoneExited;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneControlChanged OnControlChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneCaptureProgress OnCaptureProgress;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneCombatChanged OnCombatChanged;

	// Convenience API
	UFUNCTION(BlueprintCallable, Category="1_Zones|Setup")
	void SetBoxExtent(FVector NewExtent, bool bUpdateOverlaps = true);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Setup")
	void SetZoneTags(const FGameplayTagContainer& NewZoneTags); // << renamed

	UFUNCTION(BlueprintPure, Category="1_Zones|Access")
	const FGameplayTagContainer& GetZoneTags() const;

	UFUNCTION(BlueprintPure, Category="1_Zones|Access")
	USmartZoneComponent* GetSmartZone() const { return Zone; }

	UFUNCTION(BlueprintPure, Category="1_Zones|Access")
	UZoneControlComponent* GetZoneControl() const { return Control; }

protected:
	virtual void BeginPlay() override;

	// Overlap forwards
	UFUNCTION() void OnZoneBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);
	UFUNCTION() void OnZoneEnd  (UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);

	// Re-broadcast handlers
	UFUNCTION() void ForwardZoneEntered(AActor* Target, const FGameplayTagContainer& ZoneTags);
	UFUNCTION() void ForwardZoneExited (AActor* Target, const FGameplayTagContainer& ZoneTags);
	UFUNCTION() void ForwardControlChanged(EZoneControlState NewState, FGameplayTag ControlledByFaction);
	UFUNCTION() void ForwardCaptureProgress(float Progress01, FGameplayTag AttackerFaction);
	UFUNCTION() void ForwardCombatChanged(bool bInCombat);

	static bool IsPlayerActor(AActor* Other);
};
