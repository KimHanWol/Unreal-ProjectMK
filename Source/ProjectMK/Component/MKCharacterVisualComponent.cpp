#include "ProjectMK/Component/MKCharacterVisualComponent.h"

#include "AbilitySystemComponent.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Helper/MKRuntimePaperSprite.h"

namespace MKCharacterVisualComponentLocals
{
	constexpr int32 VisualAnimationAtlasCellSize = 256;

	FVector GetVisualOverrideVisualRelativeScale(bool bReverseFacingDirection, float VisualScale)
	{
		const float ClampedScale = FMath::Max(VisualScale, KINDA_SMALL_NUMBER);
		return FVector(bReverseFacingDirection ? -ClampedScale : ClampedScale, 1.f, ClampedScale);
	}

}

UMKCharacterVisualComponent::UMKCharacterVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMKCharacterVisualComponent::InitializeVisuals()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	CharacterVisualComponent = OwnerCharacter->CharacterVisualComponent;
	if (::IsValid(OwnerCharacter->GetSprite()))
	{
		BaseCharacterSpriteRelativeLocation = OwnerCharacter->GetSprite()->GetRelativeLocation();
	}
	InitializeInvincibleMaterial();
	CacheStateSpriteComponents();
	BindVisualDelegates();

	const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"));
	int32 InvincibleTagCount = 0;
	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		InvincibleTagCount = AbilitySystemComponent->GetTagCount(InvincibleTag);
	}

	HandleInvincibleTagChanged(InvincibleTag, InvincibleTagCount);
	bVisualsInitialized = true;
	UpdateVisuals();
}

void UMKCharacterVisualComponent::UpdateVisuals()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || bVisualsInitialized == false)
	{
		return;
	}

	UpdateFacingDirectionCache();

	CachedAnimationType = OwnerCharacter->ResolveCurrentCharacterAnimationType();
	OwnerCharacter->CurrentCharacterAnimationType = CachedAnimationType;

	const UPaperZDAnimSequence* CurrentAnimationSequence = nullptr;
	float PlaybackTime = 0.f;
	float PlaybackProgress = 0.f;
	bHasCachedPlaybackData = OwnerCharacter->GetCurrentAnimationPlaybackData(CurrentAnimationSequence, PlaybackTime, PlaybackProgress);
	CachedAnimationFrameIndex = OwnerCharacter->ResolveCurrentAnimationFrameIndex(CurrentAnimationSequence, PlaybackTime, PlaybackProgress);

	UpdateCharacterAnimationVisual();
	UpdateDrillShakeVisuals();
	UpdateStateSpriteVisuals();
}

void UMKCharacterVisualComponent::HandleInvincibleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	CurrentInvincibleDarkenValue = NewCount > 0 ? OwnerCharacter->InvincibleDarkenAmount : 0.f;

	if (::IsValid(InvincibleMaterialInstance))
	{
		InvincibleMaterialInstance->SetScalarParameterValue(OwnerCharacter->InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleInvincibleTagChanged: base DMI is invalid, Tag=%s, NewCount=%d"), *Tag.ToString(), NewCount);
	}

	if (::IsValid(CharacterVisualMaterialInstance))
	{
		CharacterVisualMaterialInstance->SetScalarParameterValue(OwnerCharacter->InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}
}

void UMKCharacterVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AMKCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		if (::IsValid(OwnerCharacter->GetSprite()))
		{
			OwnerCharacter->GetSprite()->SetRelativeLocation(BaseCharacterSpriteRelativeLocation);
		}
	}

	UnbindVisualDelegates();
	Super::EndPlay(EndPlayReason);
}

void UMKCharacterVisualComponent::BindVisualDelegates()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"));
		AbilitySystemComponent->RegisterGameplayTagEvent(InvincibleTag).RemoveAll(this);
		AbilitySystemComponent->RegisterGameplayTagEvent(InvincibleTag).AddUObject(this, &UMKCharacterVisualComponent::HandleInvincibleTagChanged);
	}
}

void UMKCharacterVisualComponent::UnbindVisualDelegates()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))).RemoveAll(this);
	}
}

void UMKCharacterVisualComponent::InitializeInvincibleMaterial()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(OwnerCharacter->GetSprite()) == false)
	{
		return;
	}

	InvincibleMaterialInstance = OwnerCharacter->GetSprite()->CreateDynamicMaterialInstance(0);
	if (::IsValid(InvincibleMaterialInstance) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeInvincibleMaterial: Failed to create dynamic material instance"));
		return;
	}

	InvincibleMaterialInstance->SetScalarParameterValue(OwnerCharacter->InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
}

void UMKCharacterVisualComponent::CacheStateSpriteComponents()
{
	CacheStateSpriteComponent(BalloonSpriteComponentName, BalloonStateSpriteComponent, BalloonLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillSideSpriteComponentName, DrillSideStateSpriteComponent, DrillSideLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillDownSpriteComponentName, DrillDownStateSpriteComponent, DrillDownLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillUpSpriteComponentName, DrillUpStateSpriteComponent, DrillUpLeftFacingRelativeLocation);
	if (::IsValid(DrillSideStateSpriteComponent))
	{
		DrillSideLeftFacingRelativeRotation = DrillSideStateSpriteComponent->GetRelativeRotation();
	}

	const int32 StateSpriteSortPriority = ResolveStateSpriteSortPriority();
	for (UPaperSpriteComponent* StateSpriteComponent : { BalloonStateSpriteComponent.Get(), DrillSideStateSpriteComponent.Get(), DrillDownStateSpriteComponent.Get(), DrillUpStateSpriteComponent.Get() })
	{
		if (::IsValid(StateSpriteComponent) == false)
		{
			continue;
		}

		StateSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		StateSpriteComponent->SetGenerateOverlapEvents(false);
		StateSpriteComponent->SetCanEverAffectNavigation(false);
		StateSpriteComponent->SetTranslucentSortPriority(StateSpriteSortPriority);
		StateSpriteComponent->SetSprite(nullptr);
		StateSpriteComponent->SetVisibility(false);
		StateSpriteComponent->SetHiddenInGame(true);
	}
}

void UMKCharacterVisualComponent::CacheStateSpriteComponent(FName ComponentName, TObjectPtr<UPaperSpriteComponent>& OutComponent, FVector& OutLeftFacingRelativeLocation)
{
	OutComponent = FindSpriteComponentByName(ComponentName);
	if (::IsValid(OutComponent) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("MKCharacterVisualComponent: Failed to find PaperSpriteComponent '%s' on '%s'"), *ComponentName.ToString(), *GetNameSafe(GetOwner()));
		return;
	}

	OutLeftFacingRelativeLocation = OutComponent->GetRelativeLocation();
}

void UMKCharacterVisualComponent::UpdateFacingDirectionCache()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	const float HorizontalInput = OwnerCharacter->GetCharacterDirection().X;
	if (HorizontalInput > KINDA_SMALL_NUMBER)
	{
		bFacingRight = true;
	}
	else if (HorizontalInput < -KINDA_SMALL_NUMBER)
	{
		bFacingRight = false;
	}
}

void UMKCharacterVisualComponent::UpdateCharacterAnimationVisual()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(CharacterVisualComponent) == false)
	{
		return;
	}

	OwnerCharacter->ApplySpriteRenderingOverrides(OwnerCharacter->ResolveCurrentBaseFrameSprite());
	UpdateOverrideVisualFacingDirection();

	const FCharacterDataTableRow* CharacterData = OwnerCharacter->GetCharacterData();
	if (CharacterData == nullptr || bHasCachedPlaybackData == false)
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	const TSoftObjectPtr<UTexture2D>* CharacterAnimationTexturePtr = CharacterData->AnimationTextures.FindTexture(CachedAnimationType);
	if (CharacterAnimationTexturePtr == nullptr || CharacterAnimationTexturePtr->IsNull())
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	UTexture2D* CharacterAtlasTexture = CharacterAnimationTexturePtr->LoadSynchronous();
	const UPaperSprite* CharacterSprite = ResolveAnimationAtlasSprite(CharacterAtlasTexture, CachedAnimationFrameIndex, OwnerCharacter->ResolveCurrentBasePixelsPerUnrealUnit());
	if (CharacterSprite == nullptr)
	{
		CurrentOverrideVisualScale = 1.f;
		UpdateOverrideVisualFacingDirection();
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	CurrentOverrideVisualScale = 1.f;
	UpdateOverrideVisualFacingDirection();
	OwnerCharacter->ApplySpriteRenderingOverrides(CharacterSprite);
	CharacterVisualComponent->SetSprite(const_cast<UPaperSprite*>(CharacterSprite));
	SetCharacterVisualOverrideEnabled(true);
	EnsureCharacterVisualMaterialInstance();
}

void UMKCharacterVisualComponent::SetCharacterVisualOverrideEnabled(bool bEnabled)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

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

	if (::IsValid(OwnerCharacter->GetSprite()))
	{
		OwnerCharacter->GetSprite()->SetSpriteColor(FLinearColor::White);
		OwnerCharacter->GetSprite()->SetRenderInMainPass(!bEnabled);
		OwnerCharacter->GetSprite()->SetRenderInDepthPass(false);
	}
}

void UMKCharacterVisualComponent::UpdateOverrideVisualFacingDirection()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	const UGameSettingDataAsset* GameSettings = OwnerCharacter->GetGameSettings();
	const bool bReverseFacingDirection = ::IsValid(GameSettings) && GameSettings->bReverseOverrideVisualFacingDirection;
	const FVector RelativeScale = MKCharacterVisualComponentLocals::GetVisualOverrideVisualRelativeScale(bReverseFacingDirection, CurrentOverrideVisualScale);

	if (::IsValid(CharacterVisualComponent))
	{
		CharacterVisualComponent->SetRelativeScale3D(RelativeScale);
	}
}

void UMKCharacterVisualComponent::EnsureCharacterVisualMaterialInstance()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(CharacterVisualComponent) == false || ::IsValid(OwnerCharacter->GetSprite()) == false)
	{
		return;
	}

	UMaterialInterface* BaseSpriteMaterial = OwnerCharacter->GetSprite()->GetMaterial(0);
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
		CharacterVisualMaterialInstance->SetScalarParameterValue(OwnerCharacter->InvincibleDarkenParameterName, CurrentInvincibleDarkenValue);
	}
}

void UMKCharacterVisualComponent::UpdateDrillShakeVisuals()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(OwnerCharacter->GetSprite()) == false)
	{
		CachedDrillShakeOffset = FVector::ZeroVector;
		return;
	}

	CachedDrillShakeOffset = FVector::ZeroVector;

	const bool bShouldShake = OwnerCharacter->GetDrillingVector().IsNearlyZero() == false
		&& DrillShakeOscillationSpeed > 0.f
		&& (DrillShakeHorizontalAmplitude > 0.f || DrillShakeVerticalAmplitude > 0.f);
	if (bShouldShake)
	{
		const UWorld* World = GetWorld();
		const float TimeSeconds = ::IsValid(World) ? World->GetTimeSeconds() : 0.f;

		CachedDrillShakeOffset.X = FMath::Sin(TimeSeconds * DrillShakeOscillationSpeed) * DrillShakeHorizontalAmplitude;
		CachedDrillShakeOffset.Z = FMath::Cos(TimeSeconds * DrillShakeOscillationSpeed * 1.7f) * DrillShakeVerticalAmplitude;
	}

	OwnerCharacter->GetSprite()->SetRelativeLocation(BaseCharacterSpriteRelativeLocation + CachedDrillShakeOffset);
}

void UMKCharacterVisualComponent::UpdateStateSpriteVisuals()
{
	HideAllStateSprites();
}

void UMKCharacterVisualComponent::HideAllStateSprites()
{
	for (UPaperSpriteComponent* StateSpriteComponent : { BalloonStateSpriteComponent.Get(), DrillSideStateSpriteComponent.Get(), DrillDownStateSpriteComponent.Get(), DrillUpStateSpriteComponent.Get() })
	{
		if (::IsValid(StateSpriteComponent) == false)
		{
			continue;
		}

		StateSpriteComponent->SetSprite(nullptr);
		StateSpriteComponent->SetVisibility(false);
		StateSpriteComponent->SetHiddenInGame(true);
	}
}

UPaperSpriteComponent* UMKCharacterVisualComponent::FindSpriteComponentByName(FName ComponentName) const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<UPaperSpriteComponent*> SpriteComponents;
	OwnerCharacter->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	for (UPaperSpriteComponent* SpriteComponent : SpriteComponents)
	{
		if (::IsValid(SpriteComponent) && SpriteComponent->GetFName() == ComponentName)
		{
			return SpriteComponent;
		}
	}

	return nullptr;
}

AMKCharacter* UMKCharacterVisualComponent::GetOwnerCharacter() const
{
	return Cast<AMKCharacter>(GetOwner());
}

UAbilitySystemComponent* UMKCharacterVisualComponent::GetOwnerAbilitySystemComponent() const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
}

int32 UMKCharacterVisualComponent::ResolveStateSpriteSortPriority() const
{
	return 1;
}

const UPaperSprite* UMKCharacterVisualComponent::ResolveAnimationAtlasSprite(UTexture2D* AtlasTexture, int32 AnimationFrameIndex, float PixelsPerUnrealUnit)
{
	if (::IsValid(AtlasTexture) == false)
	{
		return nullptr;
	}

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize;
	const int32 AtlasRows = AtlasTexture->GetSizeY() / MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize;
	const int32 AtlasCellCount = AtlasColumns * AtlasRows;
	if (AtlasCellCount <= 0)
	{
		return nullptr;
	}

	const int32 AtlasCellIndex = FMath::Clamp(AnimationFrameIndex, 0, AtlasCellCount - 1);
	return GetOrCreateRuntimeAtlasSprite(AtlasTexture, AtlasCellIndex, PixelsPerUnrealUnit);
}

UMKRuntimePaperSprite* UMKCharacterVisualComponent::GetOrCreateRuntimeAtlasSprite(UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(AtlasTexture) == false || AtlasCellIndex < 0)
	{
		return nullptr;
	}

	OwnerCharacter->ApplyTextureRenderingOverrides(AtlasTexture);

	const FName CacheKey = MakeRuntimeAtlasSpriteCacheKey(AtlasTexture, AtlasCellIndex, PixelsPerUnrealUnit);
	if (TObjectPtr<UMKRuntimePaperSprite>* CachedSpritePtr = RuntimeAtlasSpriteCache.Find(CacheKey))
	{
		return CachedSpritePtr->Get();
	}

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize;
	if (AtlasColumns <= 0)
	{
		return nullptr;
	}

	const FIntPoint CellOrigin(
		(AtlasCellIndex % AtlasColumns) * MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize,
		(AtlasCellIndex / AtlasColumns) * MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize);

	UMKRuntimePaperSprite* RuntimeSprite = NewObject<UMKRuntimePaperSprite>(this);
	if (::IsValid(RuntimeSprite) == false)
	{
		return nullptr;
	}

		RuntimeSprite->InitializeFromAtlasCell(
		AtlasTexture,
		CellOrigin,
		FIntPoint(MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize, MKCharacterVisualComponentLocals::VisualAnimationAtlasCellSize),
		PixelsPerUnrealUnit);

	RuntimeAtlasSpriteCache.Add(CacheKey, RuntimeSprite);
	return RuntimeSprite;
}

FName UMKCharacterVisualComponent::MakeRuntimeAtlasSpriteCacheKey(const UTexture2D* AtlasTexture, int32 AtlasCellIndex, float PixelsPerUnrealUnit) const
{
	if (AtlasTexture == nullptr)
	{
		return NAME_None;
	}

	const int32 QuantizedPixelsPerUnit = FMath::RoundToInt(PixelsPerUnrealUnit * 1000.f);
	return FName(*FString::Printf(TEXT("%s_%d_%d"), *AtlasTexture->GetPathName(), AtlasCellIndex, QuantizedPixelsPerUnit));
}
