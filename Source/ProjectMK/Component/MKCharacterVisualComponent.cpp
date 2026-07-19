#include "ProjectMK/Component/MKCharacterVisualComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Data/DataAsset/CharacterAnimationDataAsset.h"

namespace MKCharacterVisualComponentLocals
{
	constexpr float DefaultAnimationPlayRateScale = 2.f;

	FVector GetVisualOverrideVisualRelativeScale(float VisualScale)
	{
		const float ClampedScale = FMath::Max(VisualScale, KINDA_SMALL_NUMBER);
		return FVector(ClampedScale, 1.f, ClampedScale);
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

	CachedAnimationType = OwnerCharacter->GetCurrentCharacterAnimationType();
	OwnerCharacter->CurrentCharacterAnimationType = CachedAnimationType;

	UpdateCharacterAnimationVisual();
	UpdateDrillShakeVisuals();
	UpdateStateSpriteVisuals();
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
	CacheStateSpriteComponent(DrillLeftSpriteComponentName, DrillLeftStateSpriteComponent, DrillLeftLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillRightSpriteComponentName, DrillRightStateSpriteComponent, DrillRightLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillDownSpriteComponentName, DrillDownStateSpriteComponent, DrillDownLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillUpSpriteComponentName, DrillUpStateSpriteComponent, DrillUpLeftFacingRelativeLocation);

	if (::IsValid(DrillLeftStateSpriteComponent))
	{
		DrillLeftLeftFacingRelativeRotation = DrillLeftStateSpriteComponent->GetRelativeRotation();
	}

	if (::IsValid(DrillRightStateSpriteComponent))
	{
		DrillRightLeftFacingRelativeRotation = DrillRightStateSpriteComponent->GetRelativeRotation();
	}

	const int32 StateSpriteSortPriority = GetStateSpriteSortPriority();
	for (UPaperSpriteComponent* StateSpriteComponent : { BalloonStateSpriteComponent.Get(), DrillLeftStateSpriteComponent.Get(), DrillRightStateSpriteComponent.Get(), DrillDownStateSpriteComponent.Get(), DrillUpStateSpriteComponent.Get() })
	{
		if (::IsValid(StateSpriteComponent) == false)
		{
			continue;
		}

		StateSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		StateSpriteComponent->SetGenerateOverlapEvents(false);
		StateSpriteComponent->SetCanEverAffectNavigation(false);
		StateSpriteComponent->SetTranslucentSortPriority(StateSpriteSortPriority);
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
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	OwnerCharacter->ApplySpriteRenderingOverrides(OwnerCharacter->GetCurrentBaseFrameSprite());
	UpdateOverrideVisualFacingDirection();

	const FCharacterDataTableRow* CharacterData = OwnerCharacter->GetCharacterData();
	if (CharacterData == nullptr || CharacterData->AnimationDataAsset.IsNull())
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	UCharacterAnimationDataAsset* AnimationDataAsset = CharacterData->AnimationDataAsset.LoadSynchronous();
	if (::IsValid(AnimationDataAsset) == false)
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	const bool bIsDrilling = OwnerCharacter->GetDrillingVector().IsNearlyZero() == false;
	const FCharacterSpriteAnimationClip* AnimationClip = GetAnimationClipForCurrentState(AnimationDataAsset);
	const UPaperSprite* CharacterSprite = nullptr;
	if (AnimationClip != nullptr && AnimationClip->HasSprites())
	{
		const int32 FrameIndex = bIsDrilling ? 0 : CalculateAnimationFrameIndex(*AnimationClip);
		CharacterSprite = AnimationClip->GetSpriteByFrameIndex(FrameIndex);
	}

	if (CharacterSprite == nullptr)
	{
		SetCharacterVisualOverrideEnabled(false);
		return;
	}

	CurrentOverrideVisualScale = 1.f;
	UpdateOverrideVisualFacingDirection();
	OwnerCharacter->ApplySpriteRenderingOverrides(CharacterSprite);
	SetSprite(const_cast<UPaperSprite*>(CharacterSprite));
	SetCharacterVisualOverrideEnabled(true);
	EnsureCharacterVisualMaterialInstance();
}

void UMKCharacterVisualComponent::UpdateOverrideVisualFacingDirection()
{
	SetRelativeScale3D(MKCharacterVisualComponentLocals::GetVisualOverrideVisualRelativeScale(CurrentOverrideVisualScale));
}

void UMKCharacterVisualComponent::EnsureCharacterVisualMaterialInstance()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(OwnerCharacter->GetSprite()) == false)
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
		CharacterVisualMaterialInstance = CreateDynamicMaterialInstance(0, BaseSpriteMaterial);
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

	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	const FVector DrillingDirection = OwnerCharacter->GetDrillingVector();
	if (DrillingDirection.IsNearlyZero())
	{
		return;
	}

	UPaperSpriteComponent* TargetStateSpriteComponent = nullptr;
	if (FMath::Abs(DrillingDirection.X) >= FMath::Abs(DrillingDirection.Z))
	{
		TargetStateSpriteComponent = DrillingDirection.X >= 0.f ? DrillRightStateSpriteComponent.Get() : DrillLeftStateSpriteComponent.Get();
	}
	else
	{
		TargetStateSpriteComponent = DrillingDirection.Z >= 0.f ? DrillUpStateSpriteComponent.Get() : DrillDownStateSpriteComponent.Get();
	}

	if (::IsValid(TargetStateSpriteComponent))
	{
		TargetStateSpriteComponent->SetVisibility(true);
		TargetStateSpriteComponent->SetHiddenInGame(false);
	}
}

void UMKCharacterVisualComponent::HideAllStateSprites()
{
	for (UPaperSpriteComponent* StateSpriteComponent : { BalloonStateSpriteComponent.Get(), DrillLeftStateSpriteComponent.Get(), DrillRightStateSpriteComponent.Get(), DrillDownStateSpriteComponent.Get(), DrillUpStateSpriteComponent.Get() })
	{
		if (::IsValid(StateSpriteComponent) == false)
		{
			continue;
		}

		StateSpriteComponent->SetVisibility(false);
		StateSpriteComponent->SetHiddenInGame(true);
	}
}

void UMKCharacterVisualComponent::SetCharacterVisualOverrideEnabled(bool bEnabled)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	bCharacterVisualOverrideEnabled = bEnabled;

	SetVisibility(bEnabled);
	SetHiddenInGame(!bEnabled);
	if (bEnabled == false)
	{
		CurrentOverrideVisualScale = 1.f;
		UpdateOverrideVisualFacingDirection();
		SetSprite(nullptr);
	}

	if (::IsValid(OwnerCharacter->GetSprite()))
	{
		OwnerCharacter->GetSprite()->SetSpriteColor(FLinearColor::White);
		OwnerCharacter->GetSprite()->SetRenderInMainPass(!bEnabled);
		OwnerCharacter->GetSprite()->SetRenderInDepthPass(false);
	}
}

const FCharacterSpriteAnimationClip* UMKCharacterVisualComponent::GetAnimationClipForCurrentState(const UCharacterAnimationDataAsset* AnimationDataAsset) const
{
	if (::IsValid(AnimationDataAsset) == false)
	{
		return nullptr;
	}

	return AnimationDataAsset->Animations.FindClip(CachedAnimationType);
}

int32 UMKCharacterVisualComponent::CalculateAnimationFrameIndex(const FCharacterSpriteAnimationClip& AnimationClip) const
{
	if (AnimationClip.Sprites.IsEmpty())
	{
		return 0;
	}

	const float EffectivePlayRate = FMath::Max(AnimationClip.PlayRate * MKCharacterVisualComponentLocals::DefaultAnimationPlayRateScale, 0.f);
	if (EffectivePlayRate <= KINDA_SMALL_NUMBER || AnimationClip.Sprites.Num() == 1)
	{
		return 0;
	}

	const UWorld* World = GetWorld();
	const float TimeSeconds = ::IsValid(World) ? World->GetTimeSeconds() : 0.f;
	return FMath::FloorToInt(TimeSeconds * EffectivePlayRate) % AnimationClip.Sprites.Num();
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
	for (UPaperSpriteComponent* StateSpriteComponent : SpriteComponents)
	{
		if (::IsValid(StateSpriteComponent) && StateSpriteComponent->GetFName() == ComponentName)
		{
			return StateSpriteComponent;
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

int32 UMKCharacterVisualComponent::GetStateSpriteSortPriority() const
{
	return 1;
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
