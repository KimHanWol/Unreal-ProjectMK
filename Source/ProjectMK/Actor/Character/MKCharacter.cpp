// LINK

#include "ProjectMK/Actor/Character/MKCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "UObject/UnrealType.h"
#include "PaperZDAnimInstance.h"
#include "PaperZDAnimationComponent.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "AnimSequences/Players/PaperZDAnimPlayer.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/AbilitySystem/GameplayAbility/GA_Drill.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/MKCharacterVisualComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/MKRuntimePaperSprite.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

namespace
{
	constexpr int32 EquipmentOverlayAtlasCellSize = 256;
	const FName DrillingVectorVariableName(TEXT("DrillingVector"));
	constexpr float DisabledPostProcessValue = 0.f;
	constexpr float LockedExposureValue = 1.f;
	constexpr TextureFilter StableSpriteTextureFilter = TF_Bilinear;

	const TArray<EEuipmentType>& GetSupportedEquipmentTypes()
	{
		static const TArray<EEuipmentType> EquipmentTypes =
		{
			EEuipmentType::Halmet,
			EEuipmentType::Armor,
			EEuipmentType::Drill,
			EEuipmentType::Balloon,
			EEuipmentType::Gloves,
			EEuipmentType::Shoes,
		};

		return EquipmentTypes;
	}

	FName GetOverlayComponentName(EEuipmentType EquipmentType)
	{
		return FName(*FString::Printf(TEXT("EquipmentOverlay_%s"), *StaticEnum<EEuipmentType>()->GetNameStringByValue(static_cast<int64>(EquipmentType))));
	}

	FVector GetOverrideVisualRelativeScale(bool bReverseFacingDirection, float VisualScale)
	{
		const float ClampedScale = FMath::Max(VisualScale, KINDA_SMALL_NUMBER);
		return FVector(bReverseFacingDirection ? -ClampedScale : ClampedScale, 1.f, ClampedScale);
	}

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

	CharacterVisualComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("CharacterVisualComponent"));
	CharacterVisualComponent->SetupAttachment(GetSprite());
	CharacterVisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CharacterVisualComponent->SetGenerateOverlapEvents(false);
	CharacterVisualComponent->SetCanEverAffectNavigation(false);
	CharacterVisualComponent->SetRelativeLocation(FVector::ZeroVector);
	CharacterVisualComponent->SetRelativeRotation(FRotator::ZeroRotator);
	CharacterVisualComponent->SetRelativeScale3D(FVector::OneVector);
	CharacterVisualComponent->SetVisibility(false);
	CharacterVisualComponent->SetHiddenInGame(true);

	CharacterVisualLogicComponent = CreateDefaultSubobject<UMKCharacterVisualComponent>(TEXT("CharacterVisualLogicComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AttributeSet_Character = CreateDefaultSubobject<UAttributeSet_Character>(TEXT("AttributeSet_Character"));
}

UAbilitySystemComponent* AMKCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMKCharacter::SetDrillingVector(const FVector& InDrillingVector)
{
	if (bHasAppliedDrillingVectorToAnimInstance && DrillingVector.Equals(InDrillingVector))
	{
		return;
	}

	DrillingVector = InDrillingVector;
	SetAnimInstanceVectorVariable(DrillingVectorVariableName, DrillingVector);
	bHasAppliedDrillingVectorToAnimInstance = true;
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
	GrantInitialInventoryItems();
	BindEvents();
	if (::IsValid(CharacterVisualLogicComponent))
	{
		CharacterVisualLogicComponent->InitializeVisuals();
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

void AMKCharacter::GrantInitialInventoryItems()
{
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const FCharacterDataTableRow* CharacterData = GetCharacterData();
	if (CharacterData == nullptr)
	{
		return;
	}

	TSet<FName> ValidItemKeys;
	for (const FString& ItemRowName : UMKBlueprintFunctionLibrary::GetItemRowNames())
	{
		ValidItemKeys.Add(FName(*ItemRowName));
	}

	for (const FCharacterInitialInventoryEntry& InitialItem : CharacterData->InitialInventoryItems)
	{
		const FName ItemKey = InitialItem.GetItemKey();
		if (ItemKey.IsNone() || InitialItem.ItemCount <= 0)
		{
			continue;
		}

		const bool bHasItemData = ValidItemKeys.Contains(ItemKey);
		const bool bHasEquipmentData = FEquipmentItemDataTableUtil::FindEquipmentItemData(this, ItemKey) != nullptr;
		if (bHasItemData == false && bHasEquipmentData == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("GrantInitialInventoryItems: Invalid initial item key '%s' on '%s'."), *ItemKey.ToString(), *GetName());
			continue;
		}

		if (InventoryComponent->AddItem(ItemKey, InitialItem.ItemCount) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("GrantInitialInventoryItems: Failed to add '%s' x%d to '%s'."), *ItemKey.ToString(), InitialItem.ItemCount, *GetName());
			continue;
		}

		if (bHasEquipmentData && InventoryComponent->EquipItem(ItemKey) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("GrantInitialInventoryItems: Failed to equip initial equipment '%s' on '%s'."), *ItemKey.ToString(), *GetName());
		}
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
		// TODO: 드릴 어빌리티 선택 방식을 명시적으로 정리
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
	if (::IsValid(CharacterVisualLogicComponent))
	{
		CharacterVisualLogicComponent->UpdateVisuals();
	}

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

void AMKCharacter::UpdateCharacterAnimationVisual()
{
	if (::IsValid(CharacterVisualComponent) == false)
	{
		return;
	}

	ApplySpriteRenderingOverrides(ResolveCurrentBaseFrameSprite());

	const FCharacterDataTableRow* CharacterData = GetCharacterData();
	UpdateOverrideVisualFacingDirection();
	if (CharacterData == nullptr)
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	const UPaperZDAnimSequence* CurrentAnimationSequence = nullptr;
	float PlaybackTime = 0.f;
	float PlaybackProgress = 0.f;
	if (GetCurrentAnimationPlaybackData(CurrentAnimationSequence, PlaybackTime, PlaybackProgress) == false)
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	CurrentCharacterAnimationType = ResolveCurrentCharacterAnimationType();

	const TSoftObjectPtr<UTexture2D>* CharacterAnimationTexturePtr = FindCharacterAnimationTexture(*CharacterData, CurrentCharacterAnimationType);
	if (CharacterAnimationTexturePtr == nullptr || CharacterAnimationTexturePtr->IsNull())
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	const int32 AnimationFrameIndex = ResolveCurrentAnimationFrameIndex(CurrentAnimationSequence, PlaybackTime, PlaybackProgress);
	UTexture2D* CharacterAtlasTexture = CharacterAnimationTexturePtr->LoadSynchronous();
	const UPaperSprite* CharacterSprite = ResolveAnimationAtlasSprite(CharacterAtlasTexture, AnimationFrameIndex);
	if (CharacterSprite == nullptr)
	{
		CurrentOverrideVisualScale = 1.f;
		UpdateOverrideVisualFacingDirection();
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	CurrentOverrideVisualScale = 1.f;
	UpdateOverrideVisualFacingDirection();
	ApplySpriteRenderingOverrides(CharacterSprite);
	CharacterVisualComponent->SetSprite(const_cast<UPaperSprite*>(CharacterSprite));
	SetCharacterVisualOverrideEnabled(true);
	EnsureCharacterVisualMaterialInstance();
}

void AMKCharacter::SetCharacterVisualOverrideEnabled(bool bEnabled)
{
	bCharacterVisualOverrideEnabled = bEnabled;

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->SetVisibility(bEnabled);
		CharacterVisualComponent->SetHiddenInGame(!bEnabled);
		if (bEnabled == false)
		{
			CurrentOverrideVisualScale = 1.f;
			UpdateOverrideVisualFacingDirection();
			CharacterVisualComponent->SetSprite(nullptr);
		}
	}

	if (::IsValid(GetSprite()))
	{
		GetSprite()->SetSpriteColor(FLinearColor::White);
		GetSprite()->SetRenderInMainPass(!bEnabled);
		GetSprite()->SetRenderInDepthPass(false);
	}
}

void AMKCharacter::UpdateOverrideVisualFacingDirection()
{
	const UGameSettingDataAsset* GameSettings = GetGameSettings();
	const bool bReverseFacingDirection = ::IsValid(GameSettings) && GameSettings->bReverseOverrideVisualFacingDirection;
	const FVector RelativeScale = GetOverrideVisualRelativeScale(bReverseFacingDirection, CurrentOverrideVisualScale);

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->SetRelativeScale3D(RelativeScale);
	}

	for (const TPair<EEuipmentType, TObjectPtr<UPaperSpriteComponent>>& OverlayPair : EquipmentOverlayComponents)
	{
		if (::IsValid(OverlayPair.Value) == false)
		{
			continue;
		}

		OverlayPair.Value->SetRelativeScale3D(RelativeScale);
	}
}

float AMKCharacter::ResolveOverrideVisualScale(const UPaperSprite* OverrideSprite) const
{
	if (::IsValid(OverrideSprite) == false)
	{
		return 1.f;
	}

	const UPaperSprite* BaseSprite = ResolveCurrentBaseFrameSprite();
	if (::IsValid(BaseSprite) == false)
	{
		return 1.f;
	}

	const FVector2D BaseSourceSize = BaseSprite->GetSourceSize();
	const float BasePixelsPerUnit = FMath::Max(BaseSprite->GetPixelsPerUnrealUnit(), KINDA_SMALL_NUMBER);
	const float BaseLocalWidth = BaseSourceSize.X / BasePixelsPerUnit;
	const float BaseLocalHeight = BaseSourceSize.Y / BasePixelsPerUnit;
	const FVector2D OverrideSourceSize = OverrideSprite->GetSourceSize();
	const float OverridePixelsPerUnit = FMath::Max(OverrideSprite->GetPixelsPerUnrealUnit(), KINDA_SMALL_NUMBER);
	const float OverrideLocalWidth = OverrideSourceSize.X / OverridePixelsPerUnit;
	const float OverrideLocalHeight = OverrideSourceSize.Y / OverridePixelsPerUnit;

	if (BaseLocalHeight > KINDA_SMALL_NUMBER && OverrideLocalHeight > KINDA_SMALL_NUMBER)
	{
		return BaseLocalHeight / OverrideLocalHeight;
	}

	if (BaseLocalWidth > KINDA_SMALL_NUMBER && OverrideLocalWidth > KINDA_SMALL_NUMBER)
	{
		return BaseLocalWidth / OverrideLocalWidth;
	}

	return 1.f;
}

const UPaperSprite* AMKCharacter::ResolveCurrentBaseFrameSprite() const
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

float AMKCharacter::ResolveCurrentBasePixelsPerUnrealUnit() const
{
	const UPaperSprite* BaseSprite = ResolveCurrentBaseFrameSprite();
	if (::IsValid(BaseSprite))
	{
		return FMath::Max(BaseSprite->GetPixelsPerUnrealUnit(), KINDA_SMALL_NUMBER);
	}

	return 2.56f;
}

ECharacterAnimationType AMKCharacter::ResolveCurrentCharacterAnimationType() const
{
	if (DrillingVector.IsNearlyZero() == false)
	{
		if (FMath::Abs(DrillingVector.Z) > FMath::Abs(DrillingVector.X))
		{
			return DrillingVector.Z >= 0.f
				? ECharacterAnimationType::Drill_Up
				: ECharacterAnimationType::Drill_Down;
		}

		return ECharacterAnimationType::Drill_Side;
	}

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (::IsValid(MoveComp))
	{
		if (MoveComp->MovementMode == MOVE_Flying)
		{
			return ECharacterAnimationType::Fly;
		}

		if (MoveComp->IsFalling())
		{
			return ECharacterAnimationType::Fall;
		}
	}

	const float HorizontalVelocity = FMath::Abs(GetVelocity().X);
	if (HorizontalVelocity > KINDA_SMALL_NUMBER || FMath::Abs(CharacterDir.X) > KINDA_SMALL_NUMBER)
	{
		return ECharacterAnimationType::Run;
	}

	return ECharacterAnimationType::Idle;
}

const FCharacterDataTableRow* AMKCharacter::GetCharacterData() const
{
	const UDataManager* DataManager = UDataManager::Get(const_cast<AMKCharacter*>(this));
	if (::IsValid(DataManager) == false)
	{
		return nullptr;
	}

	UDataTable* CharacterDataTable = DataManager->GetDataTable(EDataTableType::Character);
	if (::IsValid(CharacterDataTable) == false)
	{
		return nullptr;
	}

	const TArray<FName> RowNames = CharacterDataTable->GetRowNames();
	if (RowNames.IsEmpty())
	{
		return nullptr;
	}

	const FName TargetRowName = CharacterDataRowKey.IsNone() == false && RowNames.Contains(CharacterDataRowKey)
		? CharacterDataRowKey
		: RowNames[0];

	if (CharacterDataTable->GetRowStruct() == FCharacterDataTableRow::StaticStruct())
	{
		return CharacterDataTable->FindRow<FCharacterDataTableRow>(TargetRowName, TEXT("GetCharacterData"));
	}

	if (CharacterDataTable->GetRowStruct() == FCharacterAnimationDataTableRow::StaticStruct())
	{
		const FCharacterAnimationDataTableRow* LegacyCharacterData = CharacterDataTable->FindRow<FCharacterAnimationDataTableRow>(TargetRowName, TEXT("GetCharacterDataLegacy"));
		if (LegacyCharacterData == nullptr)
		{
			return nullptr;
		}

		CharacterDataCompatibilityCache = LegacyCharacterData->ToCharacterDataTableRow();
		return &CharacterDataCompatibilityCache;
	}

	return nullptr;
}

const TSoftObjectPtr<UTexture2D>* AMKCharacter::FindCharacterAnimationTexture(const FCharacterDataTableRow& CharacterData, ECharacterAnimationType AnimationType) const
{
	const TSoftObjectPtr<UTexture2D>* AnimationTexturePtr = CharacterData.AnimationTextures.FindTexture(AnimationType);
	if (AnimationTexturePtr != nullptr)
	{
		return AnimationTexturePtr;
	}

	return nullptr;
}

const UPaperSprite* AMKCharacter::ResolveAnimationAtlasSprite(UTexture2D* AtlasTexture, int32 AnimationFrameIndex)
{
	if (::IsValid(AtlasTexture) == false)
	{
		return nullptr;
	}

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / EquipmentOverlayAtlasCellSize;
	const int32 AtlasRows = AtlasTexture->GetSizeY() / EquipmentOverlayAtlasCellSize;
	const int32 AtlasCellCount = AtlasColumns * AtlasRows;
	if (AtlasCellCount <= 0)
	{
		return nullptr;
	}

	const int32 AtlasCellIndex = FMath::Clamp(AnimationFrameIndex, 0, AtlasCellCount - 1);
	return GetOrCreateRuntimeAtlasSprite(AtlasTexture, AtlasCellIndex, ResolveCurrentBasePixelsPerUnrealUnit());
}

void AMKCharacter::EnsureCharacterVisualMaterialInstance()
{
	if (::IsValid(CharacterVisualComponent) == false || ::IsValid(GetSprite()) == false)
	{
		return;
	}

	UMaterialInterface* BaseSpriteMaterial = GetSprite()->GetMaterial(0);
	if (::IsValid(BaseSpriteMaterial) == false)
	{
		return;
	}

	if (::IsValid(CharacterVisualMaterialInstance) == false || CharacterVisualMaterialSource.Get() != BaseSpriteMaterial)
	{
		CharacterVisualMaterialSource = BaseSpriteMaterial;
		CharacterVisualMaterialInstance = CharacterVisualComponent->CreateDynamicMaterialInstance(0, BaseSpriteMaterial);
	}

	if (::IsValid(CharacterVisualMaterialInstance))
	{
		CharacterVisualMaterialInstance->SetScalarParameterValue(InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}
}

void AMKCharacter::InitializeEquipmentOverlayComponents()
{
	if (::IsValid(GetSprite()) == false)
	{
		return;
	}

	for (const EEuipmentType EquipmentType : GetSupportedEquipmentTypes())
	{
		if (EquipmentOverlayComponents.Contains(EquipmentType))
		{
			continue;
		}

		UPaperSpriteComponent* OverlayComponent = NewObject<UPaperSpriteComponent>(this, GetOverlayComponentName(EquipmentType));
		if (::IsValid(OverlayComponent) == false)
		{
			continue;
		}

		OverlayComponent->SetupAttachment(GetSprite());
		OverlayComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OverlayComponent->SetGenerateOverlapEvents(false);
		OverlayComponent->SetCanEverAffectNavigation(false);
		OverlayComponent->SetRelativeLocation(FVector::ZeroVector);
		OverlayComponent->SetRelativeRotation(FRotator::ZeroRotator);
		OverlayComponent->SetRelativeScale3D(FVector::OneVector);
		OverlayComponent->SetVisibility(false);
		OverlayComponent->SetHiddenInGame(true);
		OverlayComponent->RegisterComponent();

		EquipmentOverlayComponents.Add(EquipmentType, OverlayComponent);
	}

	UpdateEquipmentOverlayZOrders();
}

void AMKCharacter::RefreshEquippedOverlayItems()
{
	EquippedOverlayItemKeys.Reset();

	if (::IsValid(InventoryComponent) == false)
	{
		UpdateEquipmentOverlays();
		return;
	}

	for (const EEuipmentType EquipmentType : GetSupportedEquipmentTypes())
	{
		const FName EquippedItemKey = InventoryComponent->GetEquippedItem(EquipmentType);
		if (EquippedItemKey.IsNone() == false)
		{
			EquippedOverlayItemKeys.Add(EquipmentType, EquippedItemKey);
		}
	}

	UpdateEquipmentOverlayZOrders();
	UpdateEquipmentOverlays();
}

void AMKCharacter::UpdateEquipmentOverlays()
{
	const UPaperZDAnimSequence* CurrentAnimationSequence = nullptr;
	float PlaybackTime = 0.f;
	float PlaybackProgress = 0.f;
	GetCurrentAnimationPlaybackData(CurrentAnimationSequence, PlaybackTime, PlaybackProgress);
	const ECharacterAnimationType CurrentAnimationType = ResolveCurrentCharacterAnimationType();

	for (const EEuipmentType EquipmentType : GetSupportedEquipmentTypes())
	{
		const TObjectPtr<UPaperSpriteComponent>* OverlayComponentPtr = EquipmentOverlayComponents.Find(EquipmentType);
		if (OverlayComponentPtr == nullptr || ::IsValid(OverlayComponentPtr->Get()) == false)
		{
			continue;
		}

		UPaperSpriteComponent* OverlayComponent = OverlayComponentPtr->Get();
		const FName* EquippedItemKeyPtr = EquippedOverlayItemKeys.Find(EquipmentType);
		if (EquippedItemKeyPtr == nullptr || EquippedItemKeyPtr->IsNone())
		{
			OverlayComponent->SetSprite(nullptr);
			OverlayComponent->SetVisibility(false);
			OverlayComponent->SetHiddenInGame(true);
			continue;
		}

		const FEquipmentItemDataTableRow* EquipmentData = GetEquipmentItemData(*EquippedItemKeyPtr);
		const UPaperSprite* OverlaySprite = EquipmentData != nullptr
			? ResolveEquipmentOverlaySprite(*EquipmentData, CurrentAnimationType, CurrentAnimationSequence, PlaybackTime, PlaybackProgress)
			: nullptr;

		OverlayComponent->SetSprite(const_cast<UPaperSprite*>(OverlaySprite));
		const bool bShouldShow = OverlaySprite != nullptr;
		OverlayComponent->SetVisibility(bShouldShow);
		OverlayComponent->SetHiddenInGame(!bShouldShow);
	}
}

void AMKCharacter::UpdateEquipmentOverlayZOrders()
{
	const UGameSettingDataAsset* GameSettings = GetGameSettings();
	for (const TPair<EEuipmentType, TObjectPtr<UPaperSpriteComponent>>& OverlayPair : EquipmentOverlayComponents)
	{
		if (::IsValid(OverlayPair.Value) == false)
		{
			continue;
		}

		const int32 ZOrder = ::IsValid(GameSettings) ? GameSettings->GetEquipmentOverlayZOrder(OverlayPair.Key) : 0;
		OverlayPair.Value->SetTranslucentSortPriority(ZOrder);
	}
}

bool AMKCharacter::GetCurrentAnimationPlaybackData(const UPaperZDAnimSequence*& OutAnimationSequence, float& OutPlaybackTime, float& OutPlaybackProgress) const
{
	OutAnimationSequence = nullptr;
	OutPlaybackTime = 0.f;
	OutPlaybackProgress = 0.f;

	UPaperZDAnimInstance* CurrentAnimInstance = GetAnimInstance();
	if (::IsValid(CurrentAnimInstance) == false)
	{
		return false;
	}

	UPaperZDAnimPlayer* AnimPlayer = CurrentAnimInstance->GetPlayer();
	if (::IsValid(AnimPlayer) == false)
	{
		return false;
	}

	OutAnimationSequence = AnimPlayer->GetCurrentAnimSequence();
	OutPlaybackTime = AnimPlayer->GetCurrentPlaybackTime();
	OutPlaybackProgress = AnimPlayer->GetPlaybackProgress();
	return OutAnimationSequence != nullptr;
}

int32 AMKCharacter::ResolveCurrentAnimationFrameIndex(const UPaperZDAnimSequence* CurrentAnimationSequence, float PlaybackTime, float PlaybackProgress) const
{
	if (CurrentAnimationSequence == nullptr)
	{
		return 0;
	}

	const int32 AnimationFrameCount = FMath::Max(CurrentAnimationSequence->GetNumberOfFrames(), 1);
	int32 FrameIndex = 0;
	if (PlaybackTime >= 0.f)
	{
		FrameIndex = CurrentAnimationSequence->GetFrameAtTime(PlaybackTime);
	}
	else
	{
		FrameIndex = FMath::FloorToInt(FMath::Clamp(PlaybackProgress, 0.f, 1.f) * static_cast<float>(AnimationFrameCount));
	}

	return FMath::Clamp(FrameIndex, 0, AnimationFrameCount - 1);
}

const FEquipmentItemDataTableRow* AMKCharacter::GetEquipmentItemData(FName EquipmentKey)
{
	return FEquipmentItemDataTableUtil::FindEquipmentItemData(this, EquipmentKey);
}

const UPaperSprite* AMKCharacter::ResolveEquipmentOverlaySprite(const FEquipmentItemDataTableRow& EquipmentData, ECharacterAnimationType CurrentAnimationType, const UPaperZDAnimSequence* CurrentAnimationSequence, float PlaybackTime, float PlaybackProgress)
{
	const int32 AnimationFrameIndex = ResolveCurrentAnimationFrameIndex(CurrentAnimationSequence, PlaybackTime, PlaybackProgress);
	const TSoftObjectPtr<UTexture2D>* InlineOverlayTexturePtr = EquipmentData.AnimationOverlayTextures.FindTexture(CurrentAnimationType);
	if (InlineOverlayTexturePtr != nullptr && InlineOverlayTexturePtr->IsNull() == false)
	{
		UTexture2D* OverlayAtlasTexture = InlineOverlayTexturePtr->LoadSynchronous();
		return ResolveAnimationAtlasSprite(OverlayAtlasTexture, AnimationFrameIndex);
	}

	if (UTexture2D* IdlePreviewTexture = FEquipmentItemDataTableUtil::LoadIdlePreviewTexture(EquipmentData))
	{
		return ResolveAnimationAtlasSprite(IdlePreviewTexture, 0);
	}

	return nullptr;
}

UMKRuntimePaperSprite* AMKCharacter::GetOrCreateRuntimeAtlasSprite(UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit)
{
	if (::IsValid(AtlasTexture) == false || AtlasCellIndex < 0)
	{
		return nullptr;
	}

	ApplyTextureRenderingOverrides(AtlasTexture);

	const FName CacheKey = MakeRuntimeAtlasSpriteCacheKey(AtlasTexture, AtlasCellIndex, PixelsPerUnrealUnit);
	if (TObjectPtr<UMKRuntimePaperSprite>* CachedSpritePtr = RuntimeAtlasSpriteCache.Find(CacheKey))
	{
		return CachedSpritePtr->Get();
	}

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / EquipmentOverlayAtlasCellSize;
	if (AtlasColumns <= 0)
	{
		return nullptr;
	}

	const FIntPoint CellOrigin(
		(AtlasCellIndex % AtlasColumns) * EquipmentOverlayAtlasCellSize,
		(AtlasCellIndex / AtlasColumns) * EquipmentOverlayAtlasCellSize);

	UMKRuntimePaperSprite* RuntimeSprite = NewObject<UMKRuntimePaperSprite>(const_cast<AMKCharacter*>(this));
	if (::IsValid(RuntimeSprite) == false)
	{
		return nullptr;
	}

	RuntimeSprite->InitializeFromAtlasCell(
		AtlasTexture,
		CellOrigin,
		FIntPoint(EquipmentOverlayAtlasCellSize, EquipmentOverlayAtlasCellSize),
		PixelsPerUnrealUnit);

	RuntimeAtlasSpriteCache.Add(CacheKey, RuntimeSprite);
	return RuntimeSprite;
}

FName AMKCharacter::MakeRuntimeAtlasSpriteCacheKey(const UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit) const
{
	if (AtlasTexture == nullptr)
	{
		return NAME_None;
	}

	const int32 QuantizedPixelsPerUnit = FMath::RoundToInt(PixelsPerUnrealUnit * 1000.f);
	return FName(*FString::Printf(TEXT("%s_%d_%d"), *AtlasTexture->GetPathName(), AtlasCellIndex, QuantizedPixelsPerUnit));
}

void AMKCharacter::SetAnimInstanceVectorVariable(FName VariableName, const FVector& Value)
{
	UPaperZDAnimInstance* CurrentAnimInstance = GetAnimInstance();
	if (::IsValid(CurrentAnimInstance) == false)
	{
		return;
	}

	FStructProperty* VectorProperty = FindFProperty<FStructProperty>(CurrentAnimInstance->GetClass(), VariableName);
	if (VectorProperty == nullptr || VectorProperty->Struct != TBaseStructure<FVector>::Get())
	{
		return;
	}

	FVector* PropertyValuePtr = VectorProperty->ContainerPtrToValuePtr<FVector>(CurrentAnimInstance);
	if (PropertyValuePtr == nullptr)
	{
		return;
	}

	*PropertyValuePtr = Value;
}

void AMKCharacter::OnInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	CurrentInvincibleDarkenValue = NewCount > 0 ? InvincibleDarkenAmount : 0.f;

	if (::IsValid(InvincibleMaterialInstance) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnInvincibleTagChanged: DMI is invalid, Tag=%s, NewCount=%d"), *Tag.ToString(), NewCount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnInvincibleTagChanged: Tag=%s, NewCount=%d, Set %s=%.3f"), *Tag.ToString(), NewCount, *InvincibleDarkenParameterName.ToString(), CurrentInvincibleDarkenValue);
		InvincibleMaterialInstance->SetScalarParameterValue(InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}

	if (::IsValid(CharacterVisualMaterialInstance))
	{
		CharacterVisualMaterialInstance->SetScalarParameterValue(InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}
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
