// Source/RPGSystem/Public/GAS/RPGCue_StatNotify.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GameplayTagContainer.h"
#include "RPGCue_StatNotify.generated.h"

/**
 * Lightweight cue: reads a stat on the Target when executed and relays it to BP.
 * Great for UI/VFX that depend on a stat (e.g., scale particle size by Stamina).
 */
UCLASS(Blueprintable)
class RPGSYSTEM_API URPGCue_StatNotify : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	/** Which stat to read on target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RPG|Cue")
	FGameplayTag StatTag;

	/** Called after we read the stat (Target may be null on some cue paths). */
	UFUNCTION(BlueprintImplementableEvent, Category="RPG|Cue")
	void OnStatRead(AActor* Target, float Value) const;

	// UGameplayCueNotify_Static
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
