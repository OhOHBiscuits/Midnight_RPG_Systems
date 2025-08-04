#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces\InteractionInterface.h"
#include "Inventory\ItemDataAsset.h"
#include "BaseWorldItemActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEfficiencyChanged, float, NewEfficiency);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldItemInteracted, AActor*, Interactor);

UCLASS()
class RPGSYSTEM_API ABaseWorldItemActor : public AActor, public IInteractionInterface
{
    GENERATED_BODY()
public:
    ABaseWorldItemActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldItem")
    UItemDataAsset* ItemData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldItem")
    UStaticMeshComponent* MeshComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    TSubclassOf<UUserWidget> WidgetClass;

    //  Efficiency system toggle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldItem")
    bool bUseEfficiency = false; // Only true for stations/storage, not pickups

    // Efficiency value (only used if bUseEfficiency true)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_EfficiencyRating, Category="WorldItem")
    float EfficiencyRating = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    FGameplayTag InteractionTypeTag;
    UFUNCTION()
    virtual void OnRep_EfficiencyRating();

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnEfficiencyChanged OnEfficiencyChanged;

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnWorldItemInteracted OnWorldItemInteracted;
    
    // Apply efficiency to a resource cost/value
    UFUNCTION(BlueprintCallable, Category="WorldItem")
    float ApplyEfficiencyToCost(float BaseCost) const
    {
        if (!bUseEfficiency || EfficiencyRating <= 0.f)
            return BaseCost;
        return BaseCost / FMath::Max(EfficiencyRating, 0.01f);
    }

    UFUNCTION(BlueprintNativeEvent, Category="WorldItem")
    void SetMeshFromData();
    virtual void SetMeshFromData_Implementation();

    UFUNCTION(BlueprintCallable, Category="WorldItem")
    virtual void InitFromItemData(UItemDataAsset* NewItemData);

    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void Interact_Implementation(AActor* Interactor) override;

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerInteract(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, Category="WorldItem")
    void OnFocused();
    UFUNCTION(BlueprintNativeEvent, Category="WorldItem")
    void OnUnfocused();
    UFUNCTION(BlueprintCallable, Category="WorldItem|UI")
    virtual void TriggerWorldItemUI(AActor* Interactor);
    
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
    FGameplayTag GetInteractionTypeTag() const;
    
    UFUNCTION(Client, Reliable)
    void Client_TriggerWorldItemUI(AActor* Interactor);
    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

    

};
