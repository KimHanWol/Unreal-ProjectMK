// LINK

#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "AbilityTask/AbilityTask_Tick.h"
#include "GameplayEffect.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/GlobalConstants.h"

namespace
{
	const FGameplayTag DrillHitBlockEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Drill.HitBlock"));

	void SendDrillHitBlockEvent(
		UAbilitySystemComponent* SourceASC,
		AMKCharacter* SourceCharacter,
		ABlockBase* TargetBlock,
		const TArray<TWeakObjectPtr<AActor>>& DirectTargetActorList
	)
	{
		if (::IsValid(SourceASC) == false || ::IsValid(SourceCharacter) == false || ::IsValid(TargetBlock) == false)
		{
			return;
		}

		FGameplayEventData EventData;
		EventData.EventTag = DrillHitBlockEventTag;
		EventData.Instigator = SourceCharacter;
		EventData.Target = TargetBlock;
		EventData.OptionalObject = TargetBlock;
		EventData.ContextHandle = SourceASC->MakeEffectContext();
		EventData.ContextHandle.AddActors(DirectTargetActorList);
		SourceASC->HandleGameplayEvent(DrillHitBlockEventTag, &EventData);
	}
}

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

	SourceASC = SourceCharacter->GetAbilitySystemComponent();
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

	DrillingPower = CharacterAttributeSet->GetDrillingPower();
	DrillingDistance = CharacterAttributeSet->GetDrillingDistance();

	UAbilityTask_Tick* TickTask = UAbilityTask_Tick::CreateTickTask(this);
	TickTask->OnTick.AddDynamic(this, &UGA_Drill::Tick);
	TickTask->ReadyForActivation();
}

void UGA_Drill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	bIsDrilling = false;

	if (SourceCharacter.IsValid())
	{
		SourceCharacter->SetDrillingVector(FVector::ZeroVector);
	}

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

	UWorld* World = GetWorld();
	ULevelManagerSubsystem* LevelManagerSubsystem = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	if (::IsValid(LevelManagerSubsystem) == false)
	{
		EnableDrill(false);
		return;
	}

	const FVector Start = SourceCharacter->GetActorLocation();
	const FVector ContactTraceEnd = Start + (SourceCharDir * BLOCK_SIZE);
	DrawDebugLine(GetWorld(), Start, ContactTraceEnd, FColor::Red, false);

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(SourceCharacter.Get());

	ABlockBase* FirstHitBlock = nullptr;
	if (World->LineTraceMultiByChannel(Hits, Start, ContactTraceEnd, ECC_Visibility, Params))
	{
		for (const FHitResult& Hit : Hits)
		{
			ABlockBase* HitBlock = Cast<ABlockBase>(Hit.GetActor());
			if (::IsValid(HitBlock) && HitBlock->IsMineable() == false)
			{
				continue;
			}

			IDamageable* DamageableActor = Cast<IDamageable>(Hit.GetActor());
			if (DamageableActor == nullptr)
			{
				continue;
			}

			FirstHitBlock = HitBlock;
			break;
		}
	}

	if (::IsValid(FirstHitBlock) == false)
	{
		EnableDrill(false);
		TargetASCList.Reset();
		return;
	}

	TArray<TWeakObjectPtr<UAbilitySystemComponent>> NewTargetASCList;
	const FVector2D FirstBlockPosition = UMKBlueprintFunctionLibrary::GetBlockPosition(FirstHitBlock);
	const FVector2D BlockDirection(SourceCharDir.X, -SourceCharDir.Z);
	const int32 DrillStepCount = FMath::Max(1, FMath::CeilToInt(DrillingDistance / BLOCK_SIZE));
	const FVector RangeTraceStart = FirstHitBlock->GetActorLocation();
	const FVector RangeTraceEnd = RangeTraceStart + (SourceCharDir * ((DrillStepCount - 1) * BLOCK_SIZE));
	DrawDebugLine(GetWorld(), RangeTraceStart, RangeTraceEnd, FColor::Green, false);

	for (int32 StepIndex = 0; StepIndex < DrillStepCount; ++StepIndex)
	{
		const FVector2D BlockPosition = FirstBlockPosition + (BlockDirection * StepIndex);
		ABlockBase* TargetBlock = LevelManagerSubsystem->GetBlockAtPosition(BlockPosition);
		if (::IsValid(TargetBlock) == false)
		{
			continue;
		}

		if (TargetBlock->IsMineable() == false)
		{
			break;
		}

		IDamageable* DamageableActor = Cast<IDamageable>(TargetBlock);
		if (DamageableActor == nullptr)
		{
			continue;
		}

		if (UAbilitySystemComponent* TargetASC = DamageableActor->GetOwnerASC())
		{
			NewTargetASCList.AddUnique(TargetASC);
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
		SourceCharacter->SetDrillingVector(SourceCharDir);
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
	FHitResult SweepHitResult;
	SourceCharacter->SetActorLocation(NewLocation, true, &SweepHitResult);

	if (SweepHitResult.bBlockingHit)
	{
		InteractionDirTarget = nullptr;
		InteractingTime = 0.f;
	}
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

	const UAttributeSet_Character* CharacterAttributeSet = SourceASC.IsValid()
		? Cast<UAttributeSet_Character>(SourceASC->GetAttributeSet(UAttributeSet_Character::StaticClass()))
		: nullptr;
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return;
	}

	const float DrillingPeriod = CharacterAttributeSet->GetDrillingPeriod();
	const float EffectiveDrillingPeriod = CharacterAttributeSet->CalculateEffectiveDrillingPeriod();
	const float DrillPeriodPerOxygenIncreaseRate = CharacterAttributeSet->GetDrillPeriodPerOxygenIncreaseRate();
	if (FMath::IsNearlyZero(DrillPeriodPerOxygenIncreaseRate) == false)
	{
		const float MaxOxygenValue = CharacterAttributeSet->GetMaxOxygen();
		const float OxygenRatio = MaxOxygenValue > 0.f
			? FMath::Clamp(CharacterAttributeSet->GetCurrentOxygen() / MaxOxygenValue, 0.f, 1.f)
			: 0.f;
		const float AppliedDrillingPeriodIncreaseRate = DrillPeriodPerOxygenIncreaseRate * (1.f - OxygenRatio);
		const float AppliedDrillingSpeedIncreaseRate = EffectiveDrillingPeriod > KINDA_SMALL_NUMBER
			? (DrillingPeriod / EffectiveDrillingPeriod) - 1.f
			: 0.f;
		const bool bHasAppliedLastOxygenRate = FMath::IsNearlyZero(AppliedDrillingPeriodIncreaseRate) == false;

		UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][LastOxygen][%s] CurrentOxygen=%.2f MaxOxygen=%.2f OxygenRatio=%.3f DrillPeriodPerOxygenIncreaseRate=%.3f AppliedPeriodIncreaseRate=%.3f AppliedSpeedIncreaseRate=%.3f DrillingPeriod=%.3f EffectiveDrillingPeriod=%.3f"),
			bHasAppliedLastOxygenRate ? TEXT("On") : TEXT("Off"),
			CharacterAttributeSet->GetCurrentOxygen(),
			MaxOxygenValue,
			OxygenRatio,
			DrillPeriodPerOxygenIncreaseRate,
			AppliedDrillingPeriodIncreaseRate,
			AppliedDrillingSpeedIncreaseRate,
			DrillingPeriod,
			EffectiveDrillingPeriod);
	}

	UAbilityTask_WaitDelay* NewDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, EffectiveDrillingPeriod);
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

	TArray<TWeakObjectPtr<AActor>> DirectTargetActorList;
	for (const TWeakObjectPtr<UAbilitySystemComponent>& TargetASC : TargetASCList)
	{
		if (TargetASC.IsValid() == false)
		{
			continue;
		}

		ABlockBase* TargetBlock = Cast<ABlockBase>(TargetASC->GetOwner());
		if (::IsValid(TargetBlock) && TargetBlock->IsMineable())
		{
			DirectTargetActorList.Add(TargetBlock);
		}
	}

	for (const TWeakObjectPtr<UAbilitySystemComponent>& TargetASC : TargetASCList)
	{
		if (TargetASC.IsValid() == false)
		{
			continue;
		}

		ABlockBase* TargetBlock = Cast<ABlockBase>(TargetASC->GetOwner());
		if (::IsValid(TargetBlock) && TargetBlock->IsMineable() == false)
		{
			continue;
		}

		const UAttributeSet_Block* BlockAttributeSet = Cast<UAttributeSet_Block>(TargetASC->GetAttributeSet(UAttributeSet_Block::StaticClass()));
		if (::IsValid(BlockAttributeSet))
		{
			UE_LOG(LogTemp, Warning, TEXT("Drill Target Block Durability: %.2f"), BlockAttributeSet->GetDurability());
		}

		SendDrillHitBlockEvent(SourceASC.Get(), SourceCharacter.Get(), TargetBlock, DirectTargetActorList);
		FGameplayAbilityUtils::ApplyDamageToDurability(TargetASC.Get(), SourceASC.Get(), DrillingPower);
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

		DrillEffectHandle = FGameplayAbilityUtils::ApplyGameplayEffectToSelf(SourceASC.Get(), EffectClass);

		WaitPeriodAndMine();
	}
	else
	{
		if (SourceCharacter.IsValid())
		{
			SourceCharacter->SetDrillingVector(FVector::ZeroVector);
		}

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
