#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AreaVolumeActor.generated.h"

class UBoxComponent;

UCLASS(BlueprintType)
class RPGSYSTEM_API AAreaVolumeActor : public AActor
{
	GENERATED_BODY()
public:
	AAreaVolumeActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Area")
	UBoxComponent* Box;

	/** Tag that describes the area. Use Area.Base or Area.Player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Area")
	FGameplayTag AreaTag;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area")
	FORCEINLINE FGameplayTag GetAreaTag() const { return AreaTag; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
