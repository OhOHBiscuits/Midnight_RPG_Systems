// Source/RPGSystem/Public/GAS/RPGMMC_StatTag.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "GameplayTagContainer.h"
#include "RPGMMC_StatTag.generated.h"

/** Choose whether to read the stat from the source or the target. */
UENUM(BlueprintType)
enum class ERPGStatSource : uint8
{
	Source  UMETA(DisplayName="Source (Instigator)"),
	Target  UMETA(DisplayName="Target")
};

/**
 * Simple MMC: returns the value of a Stat (by GameplayTag) on Source or Target via StatProviderInterface.
 * Set the StatTag (and Source) on the MMC asset in the editor.
 */
UCLASS()
class RPGSYSTEM_API URPGMMC_StatTag : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	URPGMMC_StatTag();

	/** Which side to read from. */
	UPROPERTY(EditDefaultsOnly, Category="RPG|Stat")
	ERPGStatSource StatFrom = ERPGStatSource::Source;

	/** Stat tag to read (e.g., Stat.Stamina.Current) */
	UPROPERTY(EditDefaultsOnly, Category="RPG|Stat")
	FGameplayTag StatTag;

	/** Fallback if provider/tag missing. */
	UPROPERTY(EditDefaultsOnly, Category="RPG|Stat")
	float DefaultValue = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Stats")
	FGameplayTag StatToRead;


	// UGameplayModMagnitudeCalculation
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	static UObject* FindProviderFromASC(const UAbilitySystemComponent* ASC);
};
