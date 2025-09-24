// XPGrantBundle.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "XPGrantBundle.generated.h"

class USkillProgressionData;

USTRUCT(BlueprintType)
struct FSkillXPGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USkillProgressionData* Skill = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float XPGain = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float IncrementOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCarryRemainderOverride = false;
};

UCLASS(BlueprintType)
class RPGSYSTEM_API UXPGrantBundle : public UDataAsset
{
	GENERATED_BODY()
public:
	/** NEW: searchable ID so we can load bundles by tag without loading the asset first */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Base", meta=(AssetRegistrySearchable))
	FGameplayTag BundleIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Base")
	TArray<FSkillXPGrant> Grants;
};
