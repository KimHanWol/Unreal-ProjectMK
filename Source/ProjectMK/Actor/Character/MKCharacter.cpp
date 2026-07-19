// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/MKCharacterVisualComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

namespace
{
	constexpr float DisabledPostProcessValue = 0.f;
	constexpr float LockedExposureValue = 1.f;
	constexpr TextureFilter StableSpriteTextureFilter = TF_Bilinear;

	void ApplyCameraPostProcessOverrides(FPostProcessSettings& PostProcessSettings)
	{
		PostProcessSettings.bOverride_MotionBlurAmount = true;
		PostProcessSettings.MotionBlurAmount = DisabledPostProcessValue;
		PostProcessSettings.bOverride_MotionBlurMax = true;
		PostProcessSettings.MotionBlurMax = DisabledPostProcessValue;
		PostProcessSettings.bOverride_DepthOfFieldScale = true;
		PostProcessSettings.DepthOfFieldScale = DisabledPostProcessValue;
		PostProcessSettings.bOverride_BloomIntensity = true;
		PostProcessSettings.BloomIntensity = DisabledPostProcessValue;
		PostProcessSettings.bOverride_SceneFringeIntensity = true;
		PostProcessSettings.SceneFringeIntensity = DisabledPostProcessValue;
		PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
		PostProcessSettings.AutoExposureMinBrightness = LockedExposureValue;
		PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
		PostProcessSettings.AutoExposureMaxBrightness = LockedExposureValue;
	}
}

AMKCharacter::AMKCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(GetCapsuleComponent());

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetupAttachment(GetRootComponent());

	CharacterVisualComponent = CreateDefaultSubobject<UMKCharacterVisualComponent>(TEXT("CharacterVisualComponent"));
	CharacterVisualComponent->SetupAttachment(GetSprite());
	CharacterVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterVisualComponent->SetGenerateOverlapEvents(false);
	CharacterVisualComponent->SetCanEverAffectNavigation(false);
	CharacterVisualComponent->SetRelativeLocation(FVector::ZeroVector);
	CharacterVisualComponent->SetRelativeRotation(FRotator::ZeroRotator);
	CharacterVisualComponent->SetRelativeScale3D(FVector::OneVector);
	CharacterVisualComponent->SetVisibility(true);
	CharacterVisualComponent->SetHiddenInGame(true);

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

	Apply2DCameraOverrides();

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	GiveAbilities();
	InitializeCharacterAttributes();
	ApplyInitialEffects();
	BindEvents();

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->InitializeVisuals();
	}
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

void AMKCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Fly", IE_Pressed, this, &AMKCharacter::OnFly);
	PlayerInputComponent->BindAction("Fly", IE_Released, this, &AMKCharacter::OnFinishFly);

	PlayerInputComponent->BindAxis("MoveRight", this, &AMKCharacter::OnMoveRight);
	PlayerInputComponent->BindAxis("LookRight", this, &AMKCharacter::OnLookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AMKCharacter::OnLookUp);
}

void AMKCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateOxygen();

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->UpdateVisuals();
	}

	if (bIsFlying)
	{
		UpdateFlyingVerticalVelocity();
	}
}

void AMKCharacter::BindEvents()
{
	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).AddUObject(this, &AMKCharacter::OnItemCollectRangeChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).AddUObject(this, &AMKCharacter::OnCurrentHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).AddUObject(this, &AMKCharacter::OnCurrentOxygenChanged);
	}
}

void AMKCharacter::UnbindEvents()
{
	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetItemCollectRangeAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentHealthAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCurrentOxygenAttribute()).RemoveAll(this);
	}
}

UAbilitySystemComponent* AMKCharacter::GetOwnerASC()
{
	return GetAbilitySystemComponent();
}

bool AMKCharacter::CheckIsDestroyed()
{
	if (::IsValid(AttributeSet_Character) == false)
	{
		return false;
	}

	return AttributeSet_Character->GetCurrentHealth() <= 0.f;
}

void AMKCharacter::OnDestroyed()
{
}

void AMKCharacter::SetDrillingVector(const FVector& InDrillingVector)
{
	if (DrillingVector.Equals(InDrillingVector))
	{
		return;
	}

	DrillingVector = InDrillingVector;
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

void AMKCharacter::TryDrill()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	const TSubclassOf<UGameplayAbility> PrimaryDrillAbilityClass = GetPrimaryDrillAbilityClass();
	if (::IsValid(PrimaryDrillAbilityClass) == false)
	{
		return;
	}

	if (CharacterDir != FVector::ZeroVector)
	{
		// TODO: 드릴 어빌리티 선택 방식을 명시적으로 정리
		AbilitySystemComponent->TryActivateAbilityByClass(PrimaryDrillAbilityClass);
	}
	else
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(PrimaryDrillAbilityClass);
		if (AbilitySpec)
		{
			AbilitySystemComponent->CancelAbility(AbilitySpec->Ability);
		}
	}
}

void AMKCharacter::UpdateHorizontalMovement()
{
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
		return;
	}

	MoveComp->MaxWalkSpeed = AttributeSet_Character->GetMoveSpeed();
	if (FMath::IsNearlyZero(CharacterDir.X) == false)
	{
		AddMovementInput(FVector::ForwardVector, CharacterDir.X);
	}
}

void AMKCharacter::UpdateFlyingVerticalVelocity() const
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp) == false || ::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	FVector NewVelocity = MoveComp->Velocity;
	NewVelocity.Z = AttributeSet_Character->GetFlyingSpeed();
	MoveComp->Velocity = NewVelocity;
}

void AMKCharacter::Apply2DCameraOverrides()
{
	TArray<UCameraComponent*> CameraComponents;
	GetComponents<UCameraComponent>(CameraComponents);

	for (UCameraComponent* CameraComponent : CameraComponents)
	{
		if (::IsValid(CameraComponent) == false)
		{
			continue;
		}

		CameraComponent->PostProcessBlendWeight = 1.f;
		ApplyCameraPostProcessOverrides(CameraComponent->PostProcessSettings);
	}
}

void AMKCharacter::ApplyTextureRenderingOverrides(UTexture2D* Texture) const
{
	if (::IsValid(Texture) == false)
	{
		return;
	}

	bool bNeedsResourceUpdate = false;

	// The imported character assets are raster atlases, so bilinear filtering gives a steadier result than nearest.
	if (Texture->Filter != StableSpriteTextureFilter)
	{
		Texture->Filter = StableSpriteTextureFilter;
		bNeedsResourceUpdate = true;
	}

	if (Texture->NeverStream == false)
	{
		Texture->NeverStream = true;
		bNeedsResourceUpdate = true;
	}

	if (bNeedsResourceUpdate)
	{
		Texture->UpdateResource();
	}
}

void AMKCharacter::ApplySpriteRenderingOverrides(const UPaperSprite* PaperSprite) const
{
	if (::IsValid(PaperSprite) == false)
	{
		return;
	}

	ApplyTextureRenderingOverrides(PaperSprite->GetBakedTexture());
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

FVector AMKCharacter::GetCharacterDirection() const
{
	return CharacterDir;
}

FVector AMKCharacter::GetDrillingVector() const
{
	return DrillingVector;
}

TSubclassOf<UGameplayAbility> AMKCharacter::GetPrimaryDrillAbilityClass() const
{
	if (InitialGameplayAbilities.IsEmpty())
	{
		return nullptr;
	}

	return InitialGameplayAbilities[0];
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

ECharacterAnimationType AMKCharacter::GetCurrentCharacterAnimationType() const
{
	const auto GetLastIdleAnimationType = [this]() -> ECharacterAnimationType
	{
		switch (CurrentCharacterAnimationType)
		{
		case ECharacterAnimationType::Walk_Left:
		case ECharacterAnimationType::Idle_Left:
			return ECharacterAnimationType::Idle_Left;
		case ECharacterAnimationType::Walk_Right:
		case ECharacterAnimationType::Idle_Right:
			return ECharacterAnimationType::Idle_Right;
		case ECharacterAnimationType::Idle_Up:
			return ECharacterAnimationType::Idle_Up;
		default:
			return ECharacterAnimationType::Idle_Down;
		}
	};

	if (DrillingVector.IsNearlyZero() == false)
	{
		if (FMath::Abs(DrillingVector.X) >= FMath::Abs(DrillingVector.Z))
		{
			return DrillingVector.X < 0.f ? ECharacterAnimationType::Idle_Left : ECharacterAnimationType::Idle_Right;
		}

		return DrillingVector.Z < 0.f ? ECharacterAnimationType::Idle_Down : ECharacterAnimationType::Idle_Up;
	}

	if (const UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (MoveComp->MovementMode == MOVE_Flying)
		{
			if (CharacterDir.X < -KINDA_SMALL_NUMBER || GetVelocity().X < -KINDA_SMALL_NUMBER)
			{
				return ECharacterAnimationType::Idle_Left;
			}

			if (CharacterDir.X > KINDA_SMALL_NUMBER || GetVelocity().X > KINDA_SMALL_NUMBER)
			{
				return ECharacterAnimationType::Idle_Right;
			}

			return GetLastIdleAnimationType();
		}
	}

	if (CharacterDir.X < -KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Walk_Left;
	}

	if (CharacterDir.X > KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Walk_Right;
	}

	const float HorizontalVelocity = GetVelocity().X;
	if (HorizontalVelocity < -KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Walk_Left;
	}

	if (HorizontalVelocity > KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Walk_Right;
	}

	if (CharacterDir.Z < -KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Idle_Down;
	}

	if (CharacterDir.Z > KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Idle_Down;
	}

	return GetLastIdleAnimationType();
}

const UPaperSprite* AMKCharacter::GetCurrentBaseFrameSprite() const
{
	if (::IsValid(GetSprite()) == false)
	{
		return nullptr;
	}

	UPaperFlipbook* CurrentFlipbook = GetSprite()->GetFlipbook();
	if (::IsValid(CurrentFlipbook) == false)
	{
		return nullptr;
	}

	const int32 BaseFrameIndex = GetSprite()->GetPlaybackPositionInFrames();
	return CurrentFlipbook->GetSpriteAtFrame(BaseFrameIndex);
}

float AMKCharacter::GetCurrentBasePixelsPerUnrealUnit() const
{
	const UPaperSprite* BaseSprite = GetCurrentBaseFrameSprite();
	if (::IsValid(BaseSprite))
	{
		return FMath::Max(BaseSprite->GetPixelsPerUnrealUnit(), KINDA_SMALL_NUMBER);
	}

	return 2.56f;
}

const FCharacterDataTableRow* AMKCharacter::GetCharacterData() const
{
	const UDataManager* DataManager = UDataManager::Get(const_cast<AMKCharacter*>(this));
	if (::IsValid(DataManager) == false)
	{
		return nullptr;
	}

	if (DataManager->GetCharacterDataTableRow(CharacterDataRowKey, CharacterDataCompatibilityCache) == false)
	{
		return nullptr;
	}

	return &CharacterDataCompatibilityCache;
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
	UpdateHorizontalMovement();
}

void AMKCharacter::OnFly()
{
	bIsFlying = true;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp) == false)
	{
		return;
	}

	MoveComp->SetMovementMode(MOVE_Flying);
	UpdateFlyingVerticalVelocity();
}

void AMKCharacter::OnFinishFly()
{
	bIsFlying = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp) == false)
	{
		return;
	}

	if (MoveComp->MovementMode == MOVE_Flying)
	{
		MoveComp->SetMovementMode(MOVE_Falling);
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
