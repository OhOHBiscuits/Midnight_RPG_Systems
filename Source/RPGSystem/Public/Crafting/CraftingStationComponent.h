#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Progression/SkillCheckTypes.h"
#include "CraftingStationComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;
class UCheckDefinition;
class USkillProgressionData;
class UGameplayEffect;
class UCraftingRecipeDataAsset;

USTRUCT(BlueprintType)
struct FCraftItemCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	FGameplayTag ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FCraftItemOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	TObjectPtr<UItemDataAsset> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe", meta=(ClampMin="1"))
	int32 Quantity = 1;
};

UENUM(BlueprintType)
enum class ECraftPresencePolicy : uint8
{
	None,
	RequireAtStart,
	RequireAtFinish,
	RequireStartFinish
};

USTRUCT(BlueprintType)
struct FCraftingRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	FGameplayTag RecipeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	TArray<FCraftItemCost> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe")
	TArray<FCraftItemOutput> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Recipe", meta=(ClampMin="0.0"))
	float BaseTimeSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Check")
	TObjectPtr<UCheckDefinition> CheckDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Progression")
	TObjectPtr<USkillProgressionData> SkillForXP = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Progression", meta=(ClampMin="0.0"))
	float XPGain = 0.0f;

	// 0..1 at start; remainder on finish. <0 uses station default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Progression", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float XPOnStartFraction = -1.f;

	// Defaults to Instigator if unset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Progression")
	TWeakObjectPtr<AActor> XPRecipient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Presence")
	ECraftPresencePolicy PresencePolicy = ECraftPresencePolicy::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Presence", meta=(ClampMin="0.0"))
	float PresenceRadius = 600.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCraftStarted, const FCraftingRequest&, Request, float, FinalTime, const FSkillCheckResult&, Check);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCraftCompleted, const FCraftingRequest&, Request, const FSkillCheckResult&, Check, bool, bSuccess);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UCraftingStationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCraftingStationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Setup")
	FGameplayTag DomainTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Setup")
	TObjectPtr<UCheckDefinition> DefaultCheck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Setup")
	TSubclassOf<UGameplayEffect> AddXPEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Progression", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DefaultXPOnStartFraction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Output")
	TObjectPtr<UInventoryComponent> OutputInventoryOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Crafting-Output")
	bool bOutputToStationInventory = true;

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions", meta=(DefaultToSelf="Instigator"))
	bool StartCraft(AActor* Instigator, const FCraftingRequest& Request);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions", meta=(DefaultToSelf="Instigator"))
	bool StartCraftFromRecipe(AActor* Instigator, UCraftingRecipeDataAsset* Recipe);

	UFUNCTION(BlueprintCallable, Category="1_Crafting-Actions")
	void CancelCraft();

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Events")
	FOnCraftStarted OnCraftStarted;

	UPROPERTY(BlueprintAssignable, Category="1_Crafting-Events")
	FOnCraftCompleted OnCraftCompleted;

	UPROPERTY(BlueprintReadOnly, Category="1_Crafting-State")
	bool bIsCrafting = false;

	UPROPERTY(BlueprintReadOnly, Category="1_Crafting-State")
	FCraftingRequest ActiveRequest;

	UPROPERTY(BlueprintReadOnly, Category="1_Crafting-State")
	FSkillCheckResult ActiveCheck;

protected:
	virtual void BeginPlay() override;

	// Server RPC wrapper (UHT generates the wrapper call; you implement _Implementation in .cpp)
	UFUNCTION(Server, Reliable)
	void ServerStartCraft(AActor* Instigator, FCraftingRequest Request);

private:
	TWeakObjectPtr<AActor> CraftInstigator;
	FTimerHandle CraftTimer;

	UInventoryComponent* ResolveOutputInventory(AActor* Instigator) const;
	void GrantOutputs(const TArray<FCraftItemOutput>& Outputs, int32 QualityTier, AActor* Instigator);
	void GiveStartXPIfAny(const FCraftingRequest& Req, AActor* Instigator);
	void GiveFinishXPIfAny(const FCraftingRequest& Req, AActor* Instigator, bool bSuccess);

	bool IsPresenceSatisfied(AActor* Instigator, const FCraftingRequest& Req) const;
	void FinishCraft();
};
