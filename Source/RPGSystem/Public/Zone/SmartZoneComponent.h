#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SmartZoneComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneApplied, AActor*, Target, const FGameplayTagContainer&, ZoneTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneRemoved, AActor*, Target, const FGameplayTagContainer&, ZoneTags);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class RPGSYSTEM_API USmartZoneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USmartZoneComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="1_Zones|Config")
	FGameplayTagContainer ZoneTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config")
	bool bApplyOnOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config")
	bool bForceWieldProfileOnEnter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config", meta=(EditCondition="bForceWieldProfileOnEnter"))
	FGameplayTag WieldProfileOnEnter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config")
	bool bRevertWieldProfileOnExit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Zones|Config", meta=(EditCondition="bRevertWieldProfileOnExit"))
	FGameplayTag WieldProfileOnExit;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneApplied OnZoneEntered;

	UPROPERTY(BlueprintAssignable, Category="1_Zones|Events")
	FOnZoneRemoved OnZoneExited;

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void ApplyZoneToActor(AActor* Target);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void RemoveZoneFromActor(AActor* Target);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void SetZoneTags(const FGameplayTagContainer& InTags) { ZoneTags = InTags; OnRep_ZoneTags(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Zones|Queries")
	const FGameplayTagContainer& GetZoneTags() const { return ZoneTags; }

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void HandleActorBeginOverlap(AActor* Other);

	UFUNCTION(BlueprintCallable, Category="1_Zones|Actions")
	void HandleActorEndOverlap(AActor* Other);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_ZoneTags();

	void Apply_Internal(AActor* Target);
	void Remove_Internal(AActor* Target);

	void NotifySystems_Enter(AActor* Target);
	void NotifySystems_Exit(AActor* Target);

	UFUNCTION(Server, Reliable, WithValidation) void Server_ApplyZone(AActor* Target);
	UFUNCTION(Server, Reliable, WithValidation) void Server_RemoveZone(AActor* Target);
};
