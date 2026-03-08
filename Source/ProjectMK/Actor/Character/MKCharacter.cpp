// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetRootComponent(GetCapsuleComponent());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetupAttachment(GetRootComponent());

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet_Character = CreateDefaultSubobject<UAttributeSet_Character>(TEXT("AttributeSet_Character"));
}

UAbilitySystemComponent* AMKCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMKCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	InitializeInvincibleMaterial();

	GiveAbilities();
	InitializeCharacterAttributes();
	ApplyInitialEffects();
	BindEvents();
}

void AMKCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearOxygenDrainEffect();
	UnbindEvents();

	Super::EndPlay(EndPlayReason);
}

void AMKCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMKCharacter::InitializeInvincibleMaterial()
{
	UPaperFlipbookComponent* SpriteComponent = GetSprite();
	if (::IsValid(SpriteComponent) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeInvincibleMaterial: SpriteComponent is invalid"));
		return;
	}

	InvincibleMaterialInstance = SpriteComponent->CreateDynamicMaterialInstance(0);
	if (::IsValid(InvincibleMaterialInstance) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeInvincibleMaterial: Failed to create dynamic material instance"));
		return;
	}

	InvincibleMaterialInstance->SetScalarParameterValue(InvincibleDarkenParameterName, 0.f);
	UE_LOG(LogTemp, Warning, TEXT("InitializeInvincibleMaterial: Created DMI, parameter=%s set to 0.0"), *InvincibleDarkenParameterName.ToString());
}

void AMKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Fly", IE_Pressed, this, &AMKCharacter::OnFly);
	PlayerInputComponent->BindAction("Fly", IE_Released, this, &AMKCharacter::OnFinishFly);

	PlayerInputComponent->BindAxis("MoveRight", this, &AMKCharacter::OnMoveRight);
	PlayerInputComponent->BindAxis("LookRight", this, &AMKCharacter::OnLookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AMKCharacter::OnLookUp);
}

void AMKCharacter::GiveAbilities()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	for (const auto& InitialGameplayAbility : InitialGameplayAbilities)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(InitialGameplayAbility));
	}
}

void AMKCharacter::InitializeCharacterAttributes()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet_Character);
}

void AMKCharacter::ApplyInitialEffects()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& InitialGameplayEffect : InitialGameplayEffects)
	{
		if (::IsValid(InitialGameplayEffect) == false)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitialGameplayEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	RestoreOxygenToMax();
}

void AMKCharacter::BindEvents()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).AddUObject(this, &AMKCharacter::OnItemCollectRangeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &AMKCharacter::OnCurrentHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).AddUObject(this, &AMKCharacter::OnCurrentOxygenChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))).AddUObject(this, &AMKCharacter::OnInvincibleTagChanged);
}

void AMKCharacter::UnbindEvents()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).RemoveAll(this);
	AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))).RemoveAll(this);
}

UAbilitySystemComponent* AMKCharacter::GetOwnerASC()
{
	return GetAbilitySystemComponent();
}

bool AMKCharacter::CheckIsDestroyed()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return false;
	}

	const UAttributeSet_Character* CharacterAttributeSet =Cast<UAttributeSet_Character>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Character::StaticClass()));
	if (::IsValid(CharacterAttributeSet) == false)
	{
		return false;
	}

	return CharacterAttributeSet->GetCurrentHealth() <= 0.f;
}

void AMKCharacter::OnDestroyed()
{
	
}

void AMKCharacter::OnLookRight(float Value)
{
	CharacterDir.X = Value;

	TryDrill();
}

void AMKCharacter::OnLookUp(float Value)
{
	CharacterDir.Z = Value;

	TryDrill();
}

void AMKCharacter::OnMoveRight(float Value)
{
	CharacterDir.X = Value;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp) == false || ::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	if (MoveComp->IsFlying())
	{
		FVector NewVelocity = MoveComp->Velocity;
		NewVelocity.X = CharacterDir.X * AttributeSet_Character->GetMoveSpeed();

		MoveComp->Velocity = NewVelocity;
	}
	else
	{
		MoveComp->MaxWalkSpeed = AttributeSet_Character->GetMoveSpeed();
		if (FMath::IsNearlyZero(CharacterDir.X) == false)
		{
			AddMovementInput(FVector::ForwardVector, CharacterDir.X);
		}
	}
}

void AMKCharacter::OnFly()
{
	bIsFlying = true;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	MoveComp->SetMovementMode(MOVE_Flying);

	FVector NewVelocity = MoveComp->Velocity;
	NewVelocity.Z = AttributeSet_Character->GetFlyingSpeed();
	MoveComp->Velocity = NewVelocity;
}

void AMKCharacter::OnFinishFly()
{
	bIsFlying = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	if (MoveComp->MovementMode == MOVE_Flying)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
	}
}

void AMKCharacter::TryDrill()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	if (CharacterDir != FVector::ZeroVector)
	{
		//ㅁㄻㄴㄻㄴㄹ 여기 Ability 확인
		AbilitySystemComponent->TryActivateAbilityByClass(InitialGameplayAbilities[0]);
	}
	else
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(InitialGameplayAbilities[0]);
		if (AbilitySpec)
		{
			AbilitySystemComponent->CancelAbility(AbilitySpec->Ability);
		}
	}
}

void AMKCharacter::OnItemCollectRangeChanged(const FOnAttributeChangeData& Data)
{
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetGainRadius(Data.NewValue);
	}
}

void AMKCharacter::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		ApplyDamageInvincibility();
	}

	if (CheckIsDestroyed())
	{
		OnDestroyed();
	}
}

void AMKCharacter::OnCurrentOxygenChanged(const FOnAttributeChangeData& Data)
{
}

void AMKCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateOxygen();

	if (bIsFlying)
	{
		UCharacterMovementComponent* MoveComp = GetCharacterMovement();
		if (::IsValid(MoveComp) && ::IsValid(AttributeSet_Character))
		{
			FVector NewVelocity = MoveComp->Velocity;
			NewVelocity.Z = AttributeSet_Character->GetFlyingSpeed();
			MoveComp->Velocity = NewVelocity;
		}
	}
}

void AMKCharacter::UpdateOxygen()
{
	if (::IsValid(AbilitySystemComponent) == false || ::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	const UGameSettingDataAsset* GameSettings = GetGameSettings();
	if (::IsValid(GameSettings) == false)
	{
		return;
	}

	const int32 CurrentBlockDepth = GetCurrentBlockDepth();
	if (CurrentBlockDepth <= GameSettings->SurfaceBlockPositionY)
	{
		ClearOxygenDrainEffect();
		RestoreOxygenToMax();
		return;
	}

	const int32 DepthBelowSurface = CurrentBlockDepth - GameSettings->SurfaceBlockPositionY;
	const int32 DepthPerOxygenLoss = FMath::Max(1, GameSettings->DepthPerOxygenLoss);
	const float OxygenDrainPerSecond = static_cast<float>(FMath::Max(1, DepthBelowSurface / DepthPerOxygenLoss));
	ApplyOxygenDrainEffect(OxygenDrainPerSecond);
}

void AMKCharacter::ApplyOxygenDrainEffect(float OxygenDrainPerSecond)
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	const UGameSettingDataAsset* GameSettings = GetGameSettings();
	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(GameSettings) == false || ::IsValid(DataManager) == false)
	{
		return;
	}

	if (FMath::IsNearlyEqual(AppliedOxygenDrainPerSecond, OxygenDrainPerSecond) && OxygenDrainEffectHandle.IsValid())
	{
		return;
	}

	ClearOxygenDrainEffect();

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Oxygen_Drain);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	const float PeriodicDrainAmount = -(OxygenDrainPerSecond * GameSettings->OxygenDrainTickInterval);
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		const FGameplayTag ValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
		SpecHandle.Data->SetSetByCallerMagnitude(ValueTag, PeriodicDrainAmount);
		OxygenDrainEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		AppliedOxygenDrainPerSecond = OxygenDrainPerSecond;
	}
}

void AMKCharacter::ClearOxygenDrainEffect()
{
	if (::IsValid(AbilitySystemComponent) && OxygenDrainEffectHandle.IsValid())
	{
		AbilitySystemComponent->RemoveActiveGameplayEffect(OxygenDrainEffectHandle);
	}

	OxygenDrainEffectHandle.Invalidate();
	AppliedOxygenDrainPerSecond = 0.f;
}

void AMKCharacter::RestoreOxygenToMax()
{
	if (::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	const float OxygenToRestore = AttributeSet_Character->GetMaxOxygen() - AttributeSet_Character->GetCurrentOxygen();
	FDamageableUtil::ApplyOxygen(AbilitySystemComponent, OxygenToRestore);
}

int32 AMKCharacter::GetCurrentBlockDepth() const
{
	return FMath::FloorToInt(UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(GetActorLocation()).Y);
}

const UGameSettingDataAsset* AMKCharacter::GetGameSettings() const
{
	const UDataManager* DataManager = UDataManager::Get(const_cast<AMKCharacter*>(this));
	if (::IsValid(DataManager) == false)
	{
		return nullptr;
	}

	return DataManager->GetGameSettingDataAsset();
}

void AMKCharacter::OnInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (::IsValid(InvincibleMaterialInstance) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnInvincibleTagChanged: DMI is invalid, Tag=%s, NewCount=%d"), *Tag.ToString(), NewCount);
		return;
	}

	const float TargetDarkenAmount = NewCount > 0 ? InvincibleDarkenAmount : 0.f;
	UE_LOG(LogTemp, Warning, TEXT("OnInvincibleTagChanged: Tag=%s, NewCount=%d, Set %s=%.3f"), *Tag.ToString(), NewCount, *InvincibleDarkenParameterName.ToString(), TargetDarkenAmount);
	InvincibleMaterialInstance->SetScalarParameterValue(InvincibleDarkenParameterName, TargetDarkenAmount);
}

void AMKCharacter::ApplyDamageInvincibility()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Invincible);
	if (::IsValid(EffectClass) == false)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

