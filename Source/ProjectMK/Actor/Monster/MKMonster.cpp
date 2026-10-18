#include "ProjectMK/Actor/Monster/MKMonster.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/GlobalConstants.h"
#include "ProjectMK/UI/MonsterStatusWidget.h"

AMKMonster::AMKMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCapsuleComponent()->InitCapsuleSize(5.f, 7.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetSprite()->SetVisibility(false);
	GetSprite()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	Movement->SetPlaneConstraintNormal(FVector::RightVector);
	Movement->SetPlaneConstraintEnabled(true);
	Movement->bUseFlatBaseForFloorChecks = true;
	Movement->MaxStepHeight = 2.f;
	Movement->PerchRadiusThreshold = 3.5f;
	Movement->PerchAdditionalHeight = 0.f;
	Movement->BrakingDecelerationWalking = 400.f;
	Movement->MaxAcceleration = 400.f;
	Movement->bEnablePhysicsInteraction = false;
	bUseControllerRotationYaw = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UAttributeSet_Character>(TEXT("Attributes"));
	BodySprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodySprite"));
	BodySprite->SetupAttachment(GetRootComponent());
	BodySprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodySprite->SetGenerateOverlapEvents(false);
	BodySprite->SetTranslucentSortPriority(2);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (Material.Succeeded())
	{
		BodySprite->SetMaterial(0, Material.Object);
	}
	StatusWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidget"));
	StatusWidget->SetupAttachment(GetRootComponent());
	StatusWidget->SetRelativeLocation(FVector(0.f, -8.f, 12.f));
	StatusWidget->SetWidgetSpace(EWidgetSpace::Screen);
	StatusWidget->SetWidgetClass(UMonsterStatusWidget::StaticClass());
	StatusWidget->SetDrawAtDesiredSize(true);
	StatusWidget->SetPivot(FVector2D(0.5f, 1.f));
	StatusWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DrillIndicator = CreateDefaultSubobject<UArrowComponent>(TEXT("DrillIndicator"));
	DrillIndicator->SetupAttachment(GetRootComponent());
	DrillIndicator->SetArrowColor(FLinearColor(1.f, 0.3f, 0.05f));
	DrillIndicator->SetArrowLength(BLOCK_SIZE);
	DrillIndicator->SetArrowSize(0.2f);
	DrillIndicator->SetHiddenInGame(false);
	DrillIndicator->SetVisibility(false);
}

UAbilitySystemComponent* AMKMonster::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AMKMonster::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bInitialized == false || bIsDying || AppliedDamage <= 0.f)
	{
		return 0.f;
	}
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(DamageCauser);
	FGameplayAbilityUtils::ApplyDamage(AbilitySystemComponent, SourceASC, AppliedDamage);
	return AppliedDamage;
}

void AMKMonster::BeginPlay()
{
	Super::BeginPlay();
	if (InitializeMonster() == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Monster %s: missing Monster DataTable or row '%s'."), *GetName(), *MonsterDataRowKey.ToString());
		Destroy();
		return;
	}
	BindEvents();
	StatusWidget->InitWidget();
	WanderOrigin = GetActorLocation();
	GetCharacterMovement()->SetPlaneConstraintOrigin(WanderOrigin);
	ApplyIdle();
	GetWorld()->GetTimerManager().SetTimer(BehaviorTimerHandle, this, &AMKMonster::UpdateBehavior, 0.1f, true);
}

void AMKMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BehaviorTimerHandle);
	}
	UnbindEvents();
	ClearDrillTarget();
	NextContactDamageTimes.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMKMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// CharacterMovement는 매 프레임 입력을 소비한다. 행동 선택과 공격은 0.1초 타이머에서만 처리한다.
	if (bInitialized && bIsDying == false && (State == EMonsterState::Wandering || (State == EMonsterState::Angry && bAngryMoving)))
	{
		AddMovementInput(FVector::ForwardVector, MoveDirection, true);
	}
}

void AMKMonster::BindEvents()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &AMKMonster::OnCurrentHealthChanged);
}

void AMKMonster::UnbindEvents()
{
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
}

UAbilitySystemComponent* AMKMonster::GetOwnerASC()
{
	return AbilitySystemComponent;
}

bool AMKMonster::CheckIsDestroyed()
{
	return Attributes->GetCurrentHealth() <= 0.f;
}

void AMKMonster::OnDestroyed()
{
	if (bIsDying)
	{
		return;
	}
	bIsDying = true;
	GetCharacterMovement()->StopMovementImmediately();
	SetActorEnableCollision(false);
	Destroy();
}

void AMKMonster::SetMonsterDataRowKey(FName RowKey)
{
	if (HasActorBegunPlay() == false)
	{
		MonsterDataRowKey = RowKey;
	}
}

EMonsterState AMKMonster::GetMonsterState() const
{
	return State;
}

bool AMKMonster::InitializeMonster()
{
	const UDataManager* Data = UDataManager::Get(this);
	const FMonsterDataTableRow* Row = ::IsValid(Data) ? Data->GetMonsterDataTableRow(MonsterDataRowKey) : nullptr;
	if (Row == nullptr)
	{
		return false;
	}
	MonsterData = *Row;
	MonsterData.IdleDurationMin = FMath::Max(0.1f, MonsterData.IdleDurationMin);
	MonsterData.IdleDurationMax = FMath::Max(MonsterData.IdleDurationMin, MonsterData.IdleDurationMax);
	MonsterData.MoveDurationMin = FMath::Max(0.1f, MonsterData.MoveDurationMin);
	MonsterData.MoveDurationMax = FMath::Max(MonsterData.MoveDurationMin, MonsterData.MoveDurationMax);
	MonsterData.DrillCellsMin = FMath::Clamp(MonsterData.DrillCellsMin, 1, 8);
	MonsterData.DrillCellsMax = FMath::Clamp(MonsterData.DrillCellsMax, MonsterData.DrillCellsMin, 8);
	MonsterData.DrillWindup = FMath::Max(0.05f, MonsterData.DrillWindup);
	MonsterData.AngryDuration = FMath::Max(0.1f, MonsterData.AngryDuration);
	MonsterData.ContactDamage = FMath::Max(0.f, MonsterData.ContactDamage);
	MonsterData.ContactDamageInterval = FMath::Max(0.1f, MonsterData.ContactDamageInterval);
	MonsterData.ContactKnockbackSpeed = FMath::Max(0.f, MonsterData.ContactKnockbackSpeed);
	MonsterData.ContactKnockbackDuration = FMath::Max(0.f, MonsterData.ContactKnockbackDuration);
	MonsterData.ContactKnockbackUpwardSpeed = FMath::Max(0.f, MonsterData.ContactKnockbackUpwardSpeed);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->AddAttributeSetSubobject(Attributes.Get());
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetMaxHealthAttribute(), FMath::Max(1.f, MonsterData.MaxHealth));
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCurrentHealthAttribute(), Attributes->GetMaxHealth());
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetMoveSpeedAttribute(), FMath::Max(0.f, MonsterData.MoveSpeed));
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetDrillingPowerAttribute(), FMath::Max(0.1f, MonsterData.DrillingPower));
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetDrillingPeriodAttribute(), FMath::Max(0.05f, MonsterData.DrillingPeriod));
	GetCharacterMovement()->MaxWalkSpeed = Attributes->GetMoveSpeed();
	if (UPaperSprite* MonsterSprite = MonsterData.Sprite.LoadSynchronous())
	{
		BodySprite->SetSprite(MonsterSprite);
		const FVector Size = MonsterSprite->GetRenderBounds().BoxExtent * 2.f;
		const float Scale = FMath::Max(1.f, MonsterData.SpriteSize) / FMath::Max(1.f, FMath::Max(Size.X, Size.Z));
		BodySprite->SetRelativeScale3D(FVector(Scale, 1.f, Scale));
	}
	bInitialized = true;
	return true;
}

void AMKMonster::UpdateBehavior()
{
	if (bInitialized == false || bIsDying)
	{
		return;
	}
	UpdateContactDamage();
	const double Time = GetWorld()->GetTimeSeconds();
	if (Time >= StateEndTime)
	{
		if (State == EMonsterState::Idle)
		{
			ApplyWandering();
		}
		else
		{
			ApplyIdle();
		}
	}
	if (State == EMonsterState::Angry)
	{
		UpdateAngryDrilling();
	}
	else if (State == EMonsterState::Wandering && CanMoveInDirection(MoveDirection) == false)
	{
		if (CanMoveInDirection(-MoveDirection))
		{
			MoveDirection *= -1.f;
			BodySprite->SetRelativeScale3D(FVector(FMath::Abs(BodySprite->GetRelativeScale3D().X) * MoveDirection, 1.f, BodySprite->GetRelativeScale3D().Z));
		}
		else
		{
			ApplyIdle();
		}
	}
}

void AMKMonster::UpdateContactDamage()
{
	TArray<AActor*> Contacts;
	GetCapsuleComponent()->GetOverlappingActors(Contacts, AMKCharacter::StaticClass());
	const double Time = GetWorld()->GetTimeSeconds();
	for (AActor* Actor : Contacts)
	{
		AMKCharacter* Character = Cast<AMKCharacter>(Actor);
		if (::IsValid(Character) == false || MonsterData.ContactDamage <= 0.f
			|| GetCapsuleComponent()->IsOverlappingComponent(Character->GetCapsuleComponent()) == false)
		{
			continue;
		}
		double& NextDamageTime = NextContactDamageTimes.FindOrAdd(Character);
		if (Time >= NextDamageTime)
		{
			NextDamageTime = Time + MonsterData.ContactDamageInterval;
			UAbilitySystemComponent* CharacterASC = Character->GetAbilitySystemComponent();
			const float PreviousHealth = CharacterASC->GetNumericAttribute(UAttributeSet_Character::GetCurrentHealthAttribute());
			FGameplayAbilityUtils::ApplyDamage(CharacterASC, AbilitySystemComponent, MonsterData.ContactDamage);
			const float CurrentHealth = CharacterASC->GetNumericAttribute(UAttributeSet_Character::GetCurrentHealthAttribute());
			if (CurrentHealth > 0.f && CurrentHealth < PreviousHealth)
			{
				const float DeltaX = Character->GetActorLocation().X - GetActorLocation().X;
				const float Direction = FMath::IsNearlyZero(DeltaX) ? MoveDirection : FMath::Sign(DeltaX);
				Character->ApplyKnockback(Direction * MonsterData.ContactKnockbackSpeed, MonsterData.ContactKnockbackUpwardSpeed, MonsterData.ContactKnockbackDuration);
			}
		}
	}
	for (auto Iterator = NextContactDamageTimes.CreateIterator(); Iterator; ++Iterator)
	{
		if (Iterator.Key().IsValid() == false)
		{
			Iterator.RemoveCurrent();
		}
	}
}

void AMKMonster::UpdateStatusVisual()
{
	const bool bAngry = State == EMonsterState::Angry;
	BodySprite->SetSpriteColor(bAngry ? MonsterData.AngryColor : MonsterData.NormalColor);
	const TCHAR* StateName = bAngry ? TEXT("ANGRY") : (State == EMonsterState::Wandering ? TEXT("ROAM") : TEXT("IDLE"));
	if (UMonsterStatusWidget* Widget = Cast<UMonsterStatusWidget>(StatusWidget->GetUserWidgetObject()))
	{
		const FText Text = FText::Format(NSLOCTEXT("Monster", "Status", "{0}\n{1} {2}/{3}"), MonsterData.DisplayName,
			FText::FromString(StateName), FMath::RoundToInt(Attributes->GetCurrentHealth()), FMath::RoundToInt(Attributes->GetMaxHealth()));
		Widget->UpdateStatus(Text, Attributes->GetCurrentHealth() / Attributes->GetMaxHealth(),
			bAngry ? FLinearColor(1.f, 0.35f, 0.2f) : FLinearColor(0.65f, 1.f, 0.6f));
	}
}

void AMKMonster::ApplyIdle()
{
	State = EMonsterState::Idle;
	StateEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MonsterData.IdleDurationMin, MonsterData.IdleDurationMax);
	GetCharacterMovement()->Velocity.X = 0.f;
	ClearDrillTarget();
	RemainingDrillCount = 0;
	bAngryMoving = false;
	UpdateStatusVisual();
}

void AMKMonster::ApplyWandering()
{
	State = EMonsterState::Wandering;
	StateEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MonsterData.MoveDurationMin, MonsterData.MoveDurationMax);
	MoveDirection = FMath::RandBool() ? 1.f : -1.f;
	if (CanMoveInDirection(MoveDirection) == false)
	{
		MoveDirection *= -1.f;
		if (CanMoveInDirection(MoveDirection) == false)
		{
			ApplyIdle();
			return;
		}
	}
	const FVector Scale = BodySprite->GetRelativeScale3D();
	BodySprite->SetRelativeScale3D(FVector(FMath::Abs(Scale.X) * MoveDirection, 1.f, Scale.Z));
	UpdateStatusVisual();
}

void AMKMonster::ApplyAngry()
{
	// 분노 중 피격도 제한 시간과 채굴 횟수를 포함해 새 분노로 다시 시작한다.
	State = EMonsterState::Angry;
	StateEndTime = GetWorld()->GetTimeSeconds() + MonsterData.AngryDuration;
	RemainingDrillCount = FMath::RandRange(MonsterData.DrillCellsMin, MonsterData.DrillCellsMax);
	ClearDrillTarget();
	ApplyAngryWandering();
	UpdateStatusVisual();
}

void AMKMonster::ApplyAngryWandering()
{
	MoveDirection = FMath::RandBool() ? 1.f : -1.f;
	NextAngryMoveEndTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(MonsterData.MoveDurationMin, MonsterData.MoveDurationMax);
	bAngryMoving = true;
	NextDrillTime = GetWorld()->GetTimeSeconds() + MonsterData.DrillWindup;
}

void AMKMonster::UpdateAngryDrilling()
{
	const double Time = GetWorld()->GetTimeSeconds();
	if (RemainingDrillCount <= 0)
	{
		ApplyIdle();
		return;
	}
	if (GetCharacterMovement()->IsMovingOnGround() == false)
	{
		bAngryMoving = false;
		ClearDrillTarget();
		return;
	}
	if (DrillTarget.IsValid() && CanDrillTarget() == false)
	{
		ClearDrillTarget();
		ApplyAngryWandering();
	}
	if (DrillTarget.IsValid() == false)
	{
		if (Time >= NextDrillTime && TrySelectDrillTarget())
		{
			bAngryMoving = false;
			GetCharacterMovement()->Velocity.X = 0.f;
			NextDrillTime = Time + MonsterData.DrillWindup;
			return;
		}
		bAngryMoving = Time < NextAngryMoveEndTime;
		if (bAngryMoving && CanMoveInDirection(MoveDirection, true) == false)
		{
			bAngryMoving = false;
		}
		if (bAngryMoving)
		{
			const FVector Scale = BodySprite->GetRelativeScale3D();
			BodySprite->SetRelativeScale3D(FVector(FMath::Abs(Scale.X) * MoveDirection, 1.f, Scale.Z));
			return;
		}
		GetCharacterMovement()->Velocity.X = 0.f;
		if (TrySelectDrillTarget())
		{
			NextDrillTime = Time + MonsterData.DrillWindup;
		}
		else
		{
			ApplyAngryWandering();
		}
		return;
	}
	bAngryMoving = false;
	GetCharacterMovement()->Velocity.X = 0.f;
	if (Time < NextDrillTime)
	{
		return;
	}
	// 가까이 닿는 한 블록만 파괴한다. 채굴 진척은 분노 제한 시간을 연장하지 않는다.
	FGameplayAbilityUtils::ApplyDamageToDurability(DrillTarget->GetAbilitySystemComponent(), AbilitySystemComponent, Attributes->GetDrillingPower());
	NextDrillTime = Time + Attributes->GetDrillingPeriod();
	if (DrillTarget.IsValid() == false || DrillTarget->IsBreaking())
	{
		--RemainingDrillCount;
		ClearDrillTarget();
		if (RemainingDrillCount <= 0)
		{
			ApplyIdle();
		}
		else
		{
			ApplyAngryWandering();
		}
	}
}

void AMKMonster::ClearDrillTarget()
{
	DrillTarget.Reset();
	DrillIndicator->SetVisibility(false);
}

bool AMKMonster::TrySelectDrillTarget()
{
	ClearDrillTarget();
	TArray<FVector> Directions = { FVector::ForwardVector, -FVector::ForwardVector, FVector::UpVector, -FVector::UpVector };
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterDrill), false, this);
	while (Directions.IsEmpty() == false)
	{
		const int32 Index = FMath::RandRange(0, Directions.Num() - 1);
		const FVector Direction = Directions[Index];
		Directions.RemoveAtSwap(Index);
		const float Reach = (FMath::IsNearlyZero(Direction.Z) ? GetCapsuleComponent()->GetScaledCapsuleRadius()
			: GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) + 2.5f;
		FHitResult Hit;
		const FVector Location = GetActorLocation();
		if (GetWorld()->LineTraceSingleByObjectType(Hit, Location, Location + Direction * Reach, Objects, Params))
		{
			ABlockBase* Block = Cast<ABlockBase>(Hit.GetActor());
			if (::IsValid(Block) && Block->IsBreaking() == false && Block->IsMineable())
			{
				DrillTarget = Block;
				DrillIndicator->SetRelativeLocation(FVector(0.f, -4.f, 0.f));
				DrillIndicator->SetRelativeRotation(Direction.Rotation());
				DrillIndicator->SetArrowLength(Reach);
				DrillIndicator->SetVisibility(true);
				return true;
			}
		}
	}
	return false;
}

bool AMKMonster::CanMoveInDirection(float Direction, bool bApproachBlocks) const
{
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Location = GetActorLocation();
	const FVector Ahead = Location + FVector(Direction * (Radius + (bApproachBlocks ? 2.f : BLOCK_SIZE * 0.5f)), 0.f, 0.f);
	if (FMath::Abs(Ahead.X - WanderOrigin.X) > FMath::Max(1, MonsterData.WanderRadiusCells) * BLOCK_SIZE)
	{
		return false;
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterWander), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Location, Ahead, Objects, Params))
	{
		return false;
	}
	const FVector Below = Ahead - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + BLOCK_SIZE * 0.75f);
	return GetWorld()->LineTraceSingleByObjectType(Hit, Ahead, Below, Objects, Params);
}

bool AMKMonster::CanDrillTarget() const
{
	if (DrillTarget.IsValid() == false || DrillTarget->IsBreaking() || DrillTarget->IsMineable() == false)
	{
		return false;
	}
	const FVector Location = GetActorLocation();
	const FVector Delta = DrillTarget->GetActorLocation() - Location;
	const bool bHorizontal = FMath::Abs(Delta.X) >= FMath::Abs(Delta.Z);
	const FVector Direction = bHorizontal ? FVector(FMath::Sign(Delta.X), 0.f, 0.f) : FVector(0.f, 0.f, FMath::Sign(Delta.Z));
	const float Reach = (bHorizontal ? GetCapsuleComponent()->GetScaledCapsuleRadius() : GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) + 2.5f;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterDrillReach), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByObjectType(Hit, Location, Location + Direction * Reach, Objects, Params)
		&& Hit.GetActor() == DrillTarget.Get();
}

void AMKMonster::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bInitialized == false || bIsDying)
	{
		return;
	}
	if (CheckIsDestroyed())
	{
		OnDestroyed();
		return;
	}
	if (Data.NewValue < Data.OldValue)
	{
		ApplyAngry();
	}
	else
	{
		UpdateStatusVisual();
	}
}
