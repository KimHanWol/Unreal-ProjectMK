// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/Texture2D.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "TimerManager.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/MagnetRangeComponent.h"
#include "ProjectMK/Component/MKCharacterVisualComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/Interface/Interactable.h"
#include "ProjectMK/System/SkillDebugUtils.h"

namespace
{
	constexpr float DisabledPostProcessValue = 0.f;
	constexpr float LockedExposureValue = 1.f;
	constexpr TextureFilter StableSpriteTextureFilter = TF_Bilinear;
	constexpr int32 QuickSlotInputCount = 8;

	DECLARE_DELEGATE_OneParam(FQuickSlotInputDelegate, int32);

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

	MagnetRangeComponent = CreateDefaultSubobject<UMagnetRangeComponent>(TEXT("MagnetRangeComponent"));
	MagnetRangeComponent->SetupAttachment(GetRootComponent());
	MagnetRangeComponent->SetVisibility(false);

	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	ConsumableComponent = CreateDefaultSubobject<UConsumableComponent>(TEXT("ConsumableComponent"));
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));

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

	SpawnTransform = GetActorTransform();

	Apply2DCameraOverrides();

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	GiveAbilities();
	InitializeCharacterAttributes();
	BindEvents();
	ApplyInitialEffects();
	ApplySaveGame();

	if (::IsValid(MagnetRangeComponent) && ::IsValid(AttributeSet_Character))
	{
		MagnetRangeComponent->SetExpandedRadius(AttributeSet_Character->GetItemCollectRange());
	}

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->InitializeVisuals();
	}
}

void AMKCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(KnockbackRecoveryTimerHandle);
	TrySaveGame();
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
	PlayerInputComponent->BindAction("Magnet", IE_Pressed, this, &AMKCharacter::OnActivateMagnet);
	PlayerInputComponent->BindAction("Magnet", IE_Released, this, &AMKCharacter::OnDeactivateMagnet);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMKCharacter::OnInteract);

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotInputCount; ++SlotIndex)
	{
		const FName QuickSlotActionName(*FString::Printf(TEXT("QuickSlot%d"), SlotIndex + 1));
		PlayerInputComponent->BindAction<FQuickSlotInputDelegate>(QuickSlotActionName, IE_Pressed, this, &AMKCharacter::OnQuickSlot, SlotIndex);
	}

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

	if (FSkillDebugUtils::IsSkillDebugEnabled() && ::IsValid(AttributeSet_Character) && ::IsValid(InventoryComponent)
		&& InventoryComponent->IsItemCollectionActive() && AttributeSet_Character->GetItemCollectRange() > 0.f)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), AttributeSet_Character->GetItemCollectRange(), 24, FColor::Cyan, false, 0.f, 0, 1.5f);
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

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &AMKCharacter::OnInventoryChanged);
	}

	if (::IsValid(SkillComponent))
	{
		SkillComponent->OnSkillPurchasedDelegate.RemoveAll(this);
		SkillComponent->OnSkillPurchasedDelegate.AddUObject(this, &AMKCharacter::OnSkillPurchased);
	}

	if (::IsValid(CraftingComponent))
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeUnlockedDelegate.AddUObject(this, &AMKCharacter::OnRecipeChanged);
		CraftingComponent->OnRecipeCraftedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeCraftedDelegate.AddUObject(this, &AMKCharacter::OnRecipeChanged);
	}

	if (::IsValid(MagnetRangeComponent))
	{
		MagnetRangeComponent->OnExpansionFinishedDelegate.RemoveAll(this);
		MagnetRangeComponent->OnExpansionFinishedDelegate.AddUObject(this, &AMKCharacter::OnMagnetRangeExpansionFinished);
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

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
	}

	if (::IsValid(SkillComponent))
	{
		SkillComponent->OnSkillPurchasedDelegate.RemoveAll(this);
	}

	if (::IsValid(CraftingComponent))
	{
		CraftingComponent->OnRecipeUnlockedDelegate.RemoveAll(this);
		CraftingComponent->OnRecipeCraftedDelegate.RemoveAll(this);
	}

	if (::IsValid(MagnetRangeComponent))
	{
		MagnetRangeComponent->OnExpansionFinishedDelegate.RemoveAll(this);
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
	if (bIsRespawning)
	{
		return;
	}

	bIsRespawning = true;
	USoundManager::TryPlaySFX(this, ESFXType::Character_Death);
	if (::IsValid(ConsumableComponent))
	{
		ConsumableComponent->ClearRecall();
	}

	ApplyDeathPenalty();
	RespawnAtSpawnLocation();
	TrySaveGame();
	bIsRespawning = false;
}

void AMKCharacter::ApplyDeathPenalty()
{
	if (::IsValid(AttributeSet_Character) == false || ::IsValid(AbilitySystemComponent) == false)
	{
		return;
	}

	// 보험 증서가 있으면 1개 소모하고 코인·광물 손실을 막는다.
	if (::IsValid(ConsumableComponent) && ConsumableComponent->TryConsumeInsurance())
	{
		return;
	}

	const float CurrentCoin = AttributeSet_Character->GetCoin();
	const float CoinLossRatio = FMath::Clamp(AttributeSet_Character->GetDeathCoinLossRatio(), 0.f, 1.f);
	const float LostCoin = FMath::CeilToFloat(FMath::Max(0.f, CurrentCoin) * CoinLossRatio);
	const float NewCoin = FMath::Max(0.f, CurrentCoin - LostCoin);
	AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), NewCoin);

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->RemoveAllOreItems();
	}
}

void AMKCharacter::RespawnAtSpawnLocation()
{
	GetWorldTimerManager().ClearTimer(KnockbackRecoveryTimerHandle);
	KnockbackEndTime = 0.0;
	bPreserveKnockbackFacing = false;
	bIsFlying = false;
	USoundManager::TryStopSFX(this, ESFXType::Floating);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp))
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetDefaultMovementMode();
	}

	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	USoundManager::TryPlaySFX(this, ESFXType::Character_Respawn);

	if (::IsValid(AttributeSet_Character) && ::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCurrentHealthAttribute(), AttributeSet_Character->GetMaxHealth());
	}
}

void AMKCharacter::ApplyKnockback(float HorizontalVelocity, float UpwardVelocity, float RecoveryTime)
{
	if (::IsValid(GetCharacterMovement()) == false || RecoveryTime <= 0.f || FMath::IsNearlyZero(HorizontalVelocity) || bIsRespawning)
	{
		return;
	}
	KnockbackAnimationType = GetCurrentCharacterAnimationType();
	if (KnockbackAnimationType == ECharacterAnimationType::Walk_Left) { KnockbackAnimationType = ECharacterAnimationType::Idle_Left; }
	else if (KnockbackAnimationType == ECharacterAnimationType::Walk_Right) { KnockbackAnimationType = ECharacterAnimationType::Idle_Right; }
	bPreserveKnockbackFacing = true;
	KnockbackEndTime = GetWorld()->GetTimeSeconds() + RecoveryTime;
	ConsumeMovementInputVector();
	// 한 번의 충격 이후에는 중력과 충돌이 튕김 궤적을 결정한다.
	LaunchCharacter(FVector(HorizontalVelocity, 0.f, FMath::Max(0.f, UpwardVelocity)), true, true);
	GetWorldTimerManager().SetTimer(KnockbackRecoveryTimerHandle, this, &AMKCharacter::ClearKnockback, RecoveryTime, false);
}

float AMKCharacter::CalculateCurrentMoveSpeed() const
{
	if (::IsValid(AttributeSet_Character) == false)
	{
		return 0.f;
	}

	const float BaseMoveSpeed = AttributeSet_Character->GetMoveSpeed();
	if (::IsValid(InventoryComponent) == false)
	{
		return BaseMoveSpeed;
	}

	const float InventoryOccupancyRatio = InventoryComponent->GetInventoryOccupancyRatio();
	const float InventoryBurdenMoveSpeedPenaltyRate = AttributeSet_Character->GetInventoryBurdenMoveSpeedPenaltyRate();
	const float MoveSpeedMultiplier = 1.f - (InventoryOccupancyRatio * InventoryBurdenMoveSpeedPenaltyRate);
	return FMath::Max(0.f, BaseMoveSpeed * FMath::Max(0.f, MoveSpeedMultiplier));
}

void AMKCharacter::SetMenuInputBlocked(bool bBlocked)
{
	if (bIsMenuInputBlocked == bBlocked)
	{
		return;
	}
	bIsMenuInputBlocked = bBlocked;
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (bBlocked)
	{
		CharacterDir = FVector::ZeroVector;
		TryDrill();
		OnFinishFly();
		OnDeactivateMagnet();
		GetCharacterMovement()->StopMovementImmediately();
		DisableInput(PlayerController);
	}
	else
	{
		EnableInput(PlayerController);
	}
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

	// Init 이펙트가 MaxHealth 를 먼저 올리는 동안 CurrentHealth 가 0 인 채로 변경 콜백이 호출되어 사망 처리되는 것을 막는다.
	bIsApplyingInitialEffects = true;
	for (const TSubclassOf<UGameplayEffect>& InitialGameplayEffect : InitialGameplayEffects)
	{
		if (::IsValid(InitialGameplayEffect) == false)
		{
			continue;
		}

		FGameplayAbilityUtils::ApplyGameplayEffectToSelf(AbilitySystemComponent, InitialGameplayEffect);
	}
	bIsApplyingInitialEffects = false;

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

bool AMKCharacter::TryInteractNearby()
{
	UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		return false;
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	World->OverlapMultiByChannel(OverlapResults, GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(InteractSearchRadius), QueryParams);

	IInteractable* NearestInteractable = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* OverlapActor = OverlapResult.GetActor();
		IInteractable* Interactable = Cast<IInteractable>(OverlapActor);
		if (Interactable == nullptr || Interactable->CanInteract(this) == false)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), OverlapActor->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestInteractable = Interactable;
		}
	}

	return NearestInteractable != nullptr && NearestInteractable->TryInteract(this);
}

void AMKCharacter::UpdateHorizontalMovement()
{
	if (IsKnockedBack()) { return; }
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp) == false || ::IsValid(AttributeSet_Character) == false)
	{
		return;
	}

	if (MoveComp->IsFlying())
	{
		FVector NewVelocity = MoveComp->Velocity;
		NewVelocity.X = CharacterDir.X * CalculateCurrentMoveSpeed();
		MoveComp->Velocity = NewVelocity;
		return;
	}

	MoveComp->MaxWalkSpeed = CalculateCurrentMoveSpeed();
	if (FMath::IsNearlyZero(CharacterDir.X) == false)
	{
		AddMovementInput(FVector::ForwardVector, CharacterDir.X);
	}
}

void AMKCharacter::UpdateFlyingVerticalVelocity() const
{
	if (IsKnockedBack()) { return; }
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

	FGameplayAbilityUtils::ApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass);
}

void AMKCharacter::ClearKnockback()
{
	KnockbackEndTime = 0.0;
	if (bIsFlying && bIsRespawning == false && ::IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
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
	const FGameplayTag ValueTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Common.Value"));
	OxygenDrainEffectHandle = FGameplayAbilityUtils::ApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass, ValueTag, PeriodicDrainAmount);
	if (OxygenDrainEffectHandle.IsValid())
	{
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
	FGameplayAbilityUtils::ApplyOxygen(AbilitySystemComponent, OxygenToRestore);
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
		if (bIsOnSurface == false)
		{
			bIsOnSurface = true;
			USoundManager::TryPlaySFX(this, ESFXType::Oxygen_Refill);
			TrySaveGame();
		}
		return;
	}

	bIsOnSurface = false;

	const int32 DepthBelowSurface = CurrentBlockDepth - GameSettings->SurfaceBlockPositionY;
	const int32 DepthPerOxygenLoss = FMath::Max(1, GameSettings->DepthPerOxygenLoss);
	const float BaseOxygenDrainPerSecond = static_cast<float>(FMath::Max(1, DepthBelowSurface / DepthPerOxygenLoss));
	const float OxygenDrainMultiplier = AttributeSet_Character->GetOxygenDrainMultiplier();
	const float OxygenDrainPerSecond = BaseOxygenDrainPerSecond * FMath::Max(0.f, OxygenDrainMultiplier);
	ApplyOxygenDrainEffect(OxygenDrainPerSecond);
}

void AMKCharacter::ApplySaveGame()
{
	const USaveGameSubsystem* SaveGameSubsystem = USaveGameSubsystem::Get(this);
	const UMKSaveGame* SaveGame = ::IsValid(SaveGameSubsystem) ? SaveGameSubsystem->GetSaveGame() : nullptr;
	if (::IsValid(SaveGame) == false)
	{
		return;
	}

	// 스킬 효과가 인벤토리 슬롯 수를 바꿀 수 있어 스킬을 먼저 복원한다.
	if (::IsValid(SkillComponent))
	{
		SkillComponent->ApplySavedSkills(SaveGame->PurchasedSkillKeys);
	}

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), static_cast<float>(SaveGame->Coin));
	}

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->ApplySavedInventorySlots(SaveGame->InventorySlotDataList);
	}

	if (::IsValid(CraftingComponent))
	{
		CraftingComponent->ApplySavedRecipes(SaveGame->UnlockedRecipeKeys);
	}

	bHasAppliedSaveGame = true;
}

bool AMKCharacter::TrySaveGame()
{
	USaveGameSubsystem* SaveGameSubsystem = USaveGameSubsystem::Get(this);
	UMKSaveGame* SaveGame = ::IsValid(SaveGameSubsystem) ? SaveGameSubsystem->GetSaveGame() : nullptr;
	// 세이브 데이터를 적용하기 전(BeginPlay 초기화 중)에 저장하면 기존 진행이 초기값으로 덮어써진다.
	if (bHasAppliedSaveGame == false)
	{
		return false;
	}

	if (::IsValid(SaveGame) == false || ::IsValid(AttributeSet_Character) == false)
	{
		return false;
	}

	SaveGame->Coin = FMath::FloorToInt(AttributeSet_Character->GetCoin());

	if (::IsValid(SkillComponent))
	{
		SaveGame->PurchasedSkillKeys = SkillComponent->GetPurchasedSkillKeys();
	}

	if (::IsValid(InventoryComponent))
	{
		SaveGame->InventorySlotDataList = InventoryComponent->GetInventorySlotDataList();
	}

	if (::IsValid(CraftingComponent))
	{
		SaveGame->UnlockedRecipeKeys = CraftingComponent->GetPurchasedRecipeKeys();
	}

	const UWorld* World = GetWorld();
	const ULevelManagerSubsystem* LevelManagerSubsystem = ::IsValid(World) ? World->GetSubsystem<ULevelManagerSubsystem>() : nullptr;
	if (::IsValid(LevelManagerSubsystem))
	{
		SaveGame->GenerationSeed = LevelManagerSubsystem->GetLastGenerationSeed();
	}

	return SaveGameSubsystem->TrySaveGameToSlot();
}

FVector AMKCharacter::GetCharacterDirection() const
{
	return CharacterDir;
}

FVector AMKCharacter::GetDrillingVector() const
{
	return DrillingVector;
}

bool AMKCharacter::IsKnockedBack() const
{
	return GetWorld() != nullptr && GetWorld()->GetTimeSeconds() < KnockbackEndTime;
}

bool AMKCharacter::IsOnGround() const
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	return ::IsValid(MoveComp) && MoveComp->IsMovingOnGround();
}

USkillComponent* AMKCharacter::GetSkillComponent() const
{
	return SkillComponent.Get();
}

UConsumableComponent* AMKCharacter::GetConsumableComponent() const
{
	return ConsumableComponent.Get();
}

UCraftingComponent* AMKCharacter::GetCraftingComponent() const
{
	return CraftingComponent.Get();
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
	if (bPreserveKnockbackFacing) { return KnockbackAnimationType; }
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
	if (IsKnockedBack()) { return; }
	if (FMath::IsNearlyZero(Value) == false) { bPreserveKnockbackFacing = false; }
	CharacterDir.X = Value;
	TryDrill();
}

void AMKCharacter::OnLookUp(float Value)
{
	if (IsKnockedBack()) { return; }
	if (FMath::IsNearlyZero(Value) == false) { bPreserveKnockbackFacing = false; }
	CharacterDir.Z = Value;
	TryDrill();
}

void AMKCharacter::OnMoveRight(float Value)
{
	if (IsKnockedBack()) { return; }
	if (FMath::IsNearlyZero(Value) == false) { bPreserveKnockbackFacing = false; }
	CharacterDir.X = Value;
	UpdateHorizontalMovement();
}

void AMKCharacter::OnFly()
{
	bIsFlying = true;
	USoundManager::TryPlaySFX(this, ESFXType::Floating);
	if (IsKnockedBack()) { return; }

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
	USoundManager::TryStopSFX(this, ESFXType::Floating);

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

void AMKCharacter::OnActivateMagnet()
{
	bIsMagnetInputHeld = true;
	USoundManager::TryPlaySFX(this, ESFXType::Magnet_Activate);

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetItemCollectionActive(false);
	}

	if (::IsValid(MagnetRangeComponent) && ::IsValid(AttributeSet_Character))
	{
		MagnetRangeComponent->StartExpansion(AttributeSet_Character->GetItemCollectRange());
	}
}

void AMKCharacter::OnDeactivateMagnet()
{
	bIsMagnetInputHeld = false;

	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetItemCollectionActive(false);
	}

	if (::IsValid(MagnetRangeComponent))
	{
		MagnetRangeComponent->StartCollapse();
	}
}

void AMKCharacter::OnInteract()
{
	TryInteractNearby();
}

void AMKCharacter::OnQuickSlot(int32 SlotIndex)
{
	if (::IsValid(ConsumableComponent))
	{
		ConsumableComponent->TryUseInventorySlot(SlotIndex);
	}
}

void AMKCharacter::OnMagnetRangeExpansionFinished()
{
	if (bIsMagnetInputHeld && ::IsValid(InventoryComponent) && ::IsValid(AttributeSet_Character)
		&& AttributeSet_Character->GetItemCollectRange() > 0.f)
	{
		InventoryComponent->SetItemCollectionActive(true);
	}
}

void AMKCharacter::OnItemCollectRangeChanged(const FOnAttributeChangeData& Data)
{
	if (::IsValid(InventoryComponent))
	{
		InventoryComponent->SetGainRadius(Data.NewValue);
	}

	if (::IsValid(MagnetRangeComponent))
	{
		MagnetRangeComponent->SetExpandedRadius(Data.NewValue);

		if (bIsMagnetInputHeld && Data.NewValue > 0.f)
		{
			MagnetRangeComponent->StartExpansion(Data.NewValue);
		}
	}

	if (Data.NewValue <= 0.f && ::IsValid(InventoryComponent))
	{
		InventoryComponent->SetItemCollectionActive(false);
	}

	MK_SKILL_DEBUG_LOG(Warning, TEXT("[SkillDebug][Magnet] ItemCollectRange changed: %.2f -> %.2f"), Data.OldValue, Data.NewValue);
}

void AMKCharacter::OnInventoryChanged()
{
	UpdateHorizontalMovement();
}

void AMKCharacter::OnSkillPurchased(FName PurchasedSkillKeyName)
{
	TrySaveGame();
}

void AMKCharacter::OnRecipeChanged(FName RecipeKey)
{
	TrySaveGame();
}

void AMKCharacter::OnCurrentHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bIsApplyingInitialEffects)
	{
		return;
	}

	const bool bIsDestroyed = CheckIsDestroyed();
	if (Data.NewValue < Data.OldValue)
	{
		ApplyDamageInvincibility();
		if (bIsDestroyed == false)
		{
			USoundManager::TryPlaySFX(this, ESFXType::Character_Hit);
		}
	}

	if (bIsDestroyed)
	{
		OnDestroyed();
	}
}

void AMKCharacter::OnCurrentOxygenChanged(const FOnAttributeChangeData& Data)
{
	const UGameSettingDataAsset* GameSettings = GetGameSettings();
	if (::IsValid(GameSettings) == false || ::IsValid(AttributeSet_Character) == false || AttributeSet_Character->GetMaxOxygen() <= 0.f)
	{
		return;
	}

	const float LowOxygenThreshold = AttributeSet_Character->GetMaxOxygen() * GameSettings->LowOxygenWarningRatio;
	if (Data.OldValue > LowOxygenThreshold && Data.NewValue <= LowOxygenThreshold)
	{
		USoundManager::TryPlaySFX(this, ESFXType::Oxygen_Low);
	}
}
