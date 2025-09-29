// EquipmentComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UItemDataAsset;

USTRUCT(BlueprintType)
struct FEquipmentSlotDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FName SlotName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	FGameplayTagQuery AllowedItemQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Slot")
	bool bOptional = true;
};

USTRUCT(BlueprintType)
struct FEquippedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|State")
	FGameplayTag ItemIDTag;

	bool IsValid() const { return SlotTag.IsValid() && ItemIDTag.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, FGameplayTag, SlotTag, UItemDataAsset*, ItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotCleared, FGameplayTag, SlotTag);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UEquipmentComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Config")
	TArray<FEquipmentSlotDef> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_Equipped, VisibleAnywhere, BlueprintReadOnly, Category="1_Equipment|State")
	TArray<FEquippedEntry> Equipped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="1_Equipment|Config")
	bool bAutoApplyItemModifiers = true;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentChanged OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="1_Equipment|Events")
	FOnEquipmentSlotCleared OnEquippedSlotCleared;

	// Queries
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	UItemDataAsset* GetEquippedItemData(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="1_Equipment|Queries")
	bool IsEquipped(FGameplayTag SlotTag) const;

	// Actions (server-auth; client wrappers provided)
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool EquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool UnequipSlot(FGameplayTag SlotTag);

	// Client wrappers (auto RPC)
	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);

	UFUNCTION(BlueprintCallable, Category="1_Equipment|Actions")
	bool TryUnequipSlot(FGameplayTag SlotTag);

	// GAS hook point
	UFUNCTION(BlueprintNativeEvent, Category="1_Equipment|Modifiers")
	void ApplyItemModifiers(UItemDataAsset* ItemData, bool bApply);
	virtual void ApplyItemModifiers_Implementation(UItemDataAsset* ItemData, bool bApply);

	UFUNCTION(BlueprintCallable, Category="1_Equipment-Actions")
	bool TryUnequipSlotToInventory(FGameplayTag SlotTag, class UInventoryComponent* DestInventory);

	// +++ Add server stub if you keep RPCs explicit +++
	UFUNCTION(Server, Reliable)
	void Server_UnequipSlotToInventory(FGameplayTag SlotTag, class UInventoryComponent* DestInventory, class AController* Requestor);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION() void OnRep_Equipped();

	bool IsItemAllowedInSlot(const FEquipmentSlotDef& SlotDef, UItemDataAsset* ItemData) const;
	const FEquipmentSlotDef* FindSlotDef(FGameplayTag SlotTag) const;
	int32 FindEquippedIndex(FGameplayTag SlotTag) const;

	// RPCs
	UFUNCTION(Server, Reliable, WithValidation) void ServerEquipByInventoryIndex(FGameplayTag SlotTag, UInventoryComponent* FromInventory, int32 FromIndex);
	UFUNCTION(Server, Reliable, WithValidation) void ServerEquipByItemIDTag(FGameplayTag SlotTag, FGameplayTag ItemIDTag);
	UFUNCTION(Server, Reliable, WithValidation) void ServerUnequipSlot(FGameplayTag SlotTag);
};
