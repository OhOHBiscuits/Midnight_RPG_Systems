#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "RPGAbilitySystemComponent.generated.h"

/**
 * Thin ASC subclass that knows how to find your stat provider on Avatar or Owner.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGSYSTEM_API URPGAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Returns the first object on Avatar -> Owner that implements StatProviderInterface (component, actor, controller, or player state). */
	UFUNCTION(BlueprintCallable, Category="Stats")
	UObject* FindBestStatProvider() const;
};
