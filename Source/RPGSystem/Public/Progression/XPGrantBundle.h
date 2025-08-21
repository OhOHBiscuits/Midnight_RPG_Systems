#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

	// Optional overrides (use defaults from the skill asset if negative/false)
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1_Progression-Base")
	TArray<FSkillXPGrant> Grants;
};
