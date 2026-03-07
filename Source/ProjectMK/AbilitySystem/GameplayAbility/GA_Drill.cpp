// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "AbilityTask/AbilityTask_Tick.h"
#include "GameplayEffect.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"
#include "ProjectMK/System/GlobalConstants.h"

void UGA_Drill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (bIsDrilling)
    {
        return;
    }
    bIsDrilling = true;

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    MineEffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Durability_Add);
    if (::IsValid(MineEffectClass) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (ActorInfo == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    SourceCharacter = Cast<AMKCharacter>(ActorInfo->AvatarActor);
    if (SourceCharacter.IsValid() == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    SourceASC = SourceCharacter->GetComponentByClass<UAbilitySystemComponent>();
    if (SourceASC.IsValid() == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const UAttributeSet_Character* CharacterAttributeSet = Cast<UAttributeSet_Character>(SourceASC->GetAttributeSet(UAttributeSet_Character::StaticClass()));
    if (::IsValid(CharacterAttributeSet) == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    DrillingPeriod = CharacterAttributeSet->GetDrillingPeriod();
    DrillingPower = CharacterAttributeSet->GetDrillingPower();
    DrillingDistance = CharacterAttributeSet->GetDrillingDistance();

    UAbilityTask_Tick* TickTask = UAbilityTask_Tick::CreateTickTask(this);
    TickTask->OnTick.AddDynamic(this, &UGA_Drill::Tick);
    TickTask->ReadyForActivation();
}

void UGA_Drill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    bIsDrilling = false;

    if (DelayTask.IsValid())
    {
        DelayTask->EndTask();
        DelayTask = nullptr;
    }

    EnableDrill(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Drill::Tick(float DeltaTime)
{
    Tick_UpdateTarget();
    Tick_UpdateSourcePosition(DeltaTime);
}

void UGA_Drill::Tick_UpdateTarget()
{
    if (SourceCharacter.IsValid() == false)
    {
        EnableDrill(false);
        return;
    }

    if (SourceASC.IsValid() == false)
    {
        EnableDrill(false);
        return;
    }

    const FVector& SourceCharDir = SourceCharacter->GetCharacterDirection();
    if (SourceCharDir == FVector::ZeroVector || 
        SourceCharDir.Size() > 1.f) 
    {
        EnableDrill(false);
        return;
    }

    FVector Start = SourceCharacter->GetActorLocation();
    FVector End = Start + SourceCharDir * DrillingDistance;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(SourceCharacter.Get());

    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false);

    TArray<TWeakObjectPtr<UAbilitySystemComponent>> NewTargetASCList;
    if (GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, Params))
    {
        for (const auto& Hit : Hits)
        {
            IDamageable* DamageableActor = Cast<IDamageable>(Hit.GetActor());
            if (DamageableActor)
            {
                NewTargetASCList.Add(DamageableActor->GetOwnerASC());
            }
        }
    }

    if (NewTargetASCList.IsEmpty() ||
        (TargetASCList.IsEmpty() == false && NewTargetASCList[0] != TargetASCList[0]))
    {
        EnableDrill(false);
    }
    TargetASCList = NewTargetASCList;

    if (NewTargetASCList.IsEmpty() == false)
    {
        EnableDrill(true);
    }
}

void UGA_Drill::Tick_UpdateSourcePosition(float DeltaTime)
{
    if (SourceCharacter.IsValid() == false)
    {
        return;
    }

    //아래쪽으로 갈 때만 동작
    const FVector& SourceCharDir = SourceCharacter->GetCharacterDirection();
    if (SourceCharDir.X != 0.f)
    {
        InteractionDirTarget = nullptr;
        return;
    }

    if (TargetASCList.IsEmpty())
    {
        InteractionDirTarget = nullptr;
        return;
    }

    if (TargetASCList[0].IsValid() == false)
    {
        InteractionDirTarget = nullptr;
        return;
    }

    if (InteractionDirTarget != TargetASCList[0]->GetOwner())
    {
        InteractingTime = 0.f;
        InteractionStartPoint = SourceCharacter->GetActorLocation();
    }
    InteractionDirTarget = TargetASCList[0]->GetOwner();
    InteractingTime += DeltaTime;

    if (InteractionDirTarget.IsValid() == false)
    {
        return;
    }

    const FVector& InteractedActorLocation = InteractionDirTarget->GetActorLocation();
    FVector TargetLocation = InteractedActorLocation - SourceCharDir * BLOCK_SIZE;
    TargetLocation.Z = SourceCharacter->GetActorLocation().Z;

    if (SourceCharacter->GetActorLocation() == TargetLocation)
    {
        return;
    }

    float Alpha = FMath::Clamp(InteractingTime / InteractionMoveDuration, 0.0f, 1.0f);
    FVector NewLocation = FMath::Lerp(InteractionStartPoint, TargetLocation, Alpha);
    SourceCharacter->SetActorLocation(NewLocation);
}

void UGA_Drill::WaitPeriodAndMine()
{
    if (DelayTask.IsValid())
    {
        DelayTask->EndTask();
        DelayTask = nullptr;
    }

    if (DrillEffectHandle.IsValid() == false)
    {
        return;
    }

    UAbilityTask_WaitDelay* NewDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DrillingPeriod);
    DelayTask = NewDelayTask;

    DelayTask->OnFinish.AddDynamic(this, &UGA_Drill::Drill_Instant);
    DelayTask->ReadyForActivation();
}

void UGA_Drill::Drill_Instant()
{
    if (DrillEffectHandle.IsValid() == false)
    {
        return;
    }

    if (SourceASC.IsValid() == false)
    {
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("Drill!"));

    for (const auto& TargetASC : TargetASCList)
    {
        if (TargetASC.IsValid())
        {
            FDamageableUtil::ApplyDamageToDurability(TargetASC.Get(), SourceASC.Get(), DrillingPower);
        }
    }

    if (DelayTask.IsValid())
    {
        DelayTask = nullptr;
    }

    WaitPeriodAndMine();
}

void UGA_Drill::EnableDrill(bool bEnable)
{
    if (SourceASC.IsValid() == false)
    {
        return;
    }

    const UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::State_Drill);
    if (::IsValid(EffectClass) == false)
    {
        return;
    }

    if (bEnable)
    {
        if (DrillEffectHandle.IsValid())
        {
            return;
        }

        FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, SourceASC->MakeEffectContext());
        if (SpecHandle.IsValid())
        {
            DrillEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }

        WaitPeriodAndMine();
    }
    else
    {
        if (DelayTask.IsValid())
        {
            DelayTask->EndTask();
            DelayTask = nullptr;
        }

        if (DrillEffectHandle.IsValid() == false)
        {
            return;
        }

        SourceASC->RemoveActiveGameplayEffect(DrillEffectHandle);
        DrillEffectHandle.Invalidate();
    }
}

