//LINK

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_Tick.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityTaskTickDelegate, float, DeltaTime);

UCLASS()
class PROJECTMK_API UAbilityTask_Tick : public UAbilityTask
{
    GENERATED_BODY()

public:
    UAbilityTask_Tick();

    UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility"))
    static UAbilityTask_Tick* CreateTickTask(UGameplayAbility* OwningAbility);

    UPROPERTY(BlueprintAssignable)
    FAbilityTaskTickDelegate OnTick;

protected:
    virtual void TickTask(float DeltaTime) override;
    virtual void OnDestroy(bool AbilityEnded) override;
};

