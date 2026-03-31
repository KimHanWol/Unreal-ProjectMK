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
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Helper/MKRuntimePaperSprite.h"
#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"

namespace MKCharacterVisualComponentLocals
{
	constexpr int32 VisualEquipmentOverlayAtlasCellSize = 256;

	const TArray<EEuipmentType>& GetVisualSupportedEquipmentTypes()
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

	FName GetVisualOverlayComponentName(EEuipmentType EquipmentType)
	{
		return FName(*FString::Printf(TEXT("EquipmentOverlay_%s"), *StaticEnum<EEuipmentType>()->GetNameStringByValue(static_cast<int64>(EquipmentType))));
	}

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
	InitializeInvincibleMaterial();
	InitializeEquipmentOverlayComponents();
	CacheStateSpriteComponents();
	BindVisualDelegates();
	RefreshEquippedOverlayItems();
	UpdateEquipmentOverlayZOrders();

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
	UpdateEquipmentOverlays();
	UpdateStateSpriteVisuals();
}

void UMKCharacterVisualComponent::HandleInventoryChanged()
{
	if (bVisualsInitialized == false)
	{
		return;
	}

	RefreshEquippedOverlayItems();
	UpdateEquipmentOverlayZOrders();
	UpdateVisuals();
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

	if (UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
		InventoryComponent->OnInventoryChangedDelegate.AddUObject(this, &UMKCharacterVisualComponent::HandleInventoryChanged);
	}
}

void UMKCharacterVisualComponent::UnbindVisualDelegates()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))).RemoveAll(this);
	}

	if (UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		InventoryComponent->OnInventoryChangedDelegate.RemoveAll(this);
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

void UMKCharacterVisualComponent::InitializeEquipmentOverlayComponents()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false || ::IsValid(OwnerCharacter->GetSprite()) == false)
	{
		return;
	}

	for (const EEuipmentType EquipmentType : MKCharacterVisualComponentLocals::GetVisualSupportedEquipmentTypes())
	{
		if (EquipmentOverlayComponents.Contains(EquipmentType))
		{
			continue;
		}

		UPaperSpriteComponent* OverlayComponent = NewObject<UPaperSpriteComponent>(OwnerCharacter, MKCharacterVisualComponentLocals::GetVisualOverlayComponentName(EquipmentType));
		if (::IsValid(OverlayComponent) == false)
		{
			continue;
		}

		OverlayComponent->SetupAttachment(OwnerCharacter->GetSprite());
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

void UMKCharacterVisualComponent::CacheStateSpriteComponents()
{
	CacheStateSpriteComponent(BalloonSpriteComponentName, BalloonStateSpriteComponent, BalloonLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillSideSpriteComponentName, DrillSideStateSpriteComponent, DrillSideLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillDownSpriteComponentName, DrillDownStateSpriteComponent, DrillDownLeftFacingRelativeLocation);
	CacheStateSpriteComponent(DrillUpSpriteComponentName, DrillUpStateSpriteComponent, DrillUpLeftFacingRelativeLocation);

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

	for (const TPair<EEuipmentType, TObjectPtr<UPaperSpriteComponent>>& OverlayPair : EquipmentOverlayComponents)
	{
		if (::IsValid(OverlayPair.Value) == false)
		{
			continue;
		}

		OverlayPair.Value->SetRelativeScale3D(RelativeScale);
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

void UMKCharacterVisualComponent::RefreshEquippedOverlayItems()
{
	EquippedOverlayItemKeys.Reset();

	if (UInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		for (const EEuipmentType EquipmentType : MKCharacterVisualComponentLocals::GetVisualSupportedEquipmentTypes())
		{
			const FName EquippedItemKey = InventoryComponent->GetEquippedItem(EquipmentType);
			if (EquippedItemKey.IsNone() == false)
			{
				EquippedOverlayItemKeys.Add(EquipmentType, EquippedItemKey);
			}
		}
	}
}

void UMKCharacterVisualComponent::UpdateEquipmentOverlays()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	for (const EEuipmentType EquipmentType : MKCharacterVisualComponentLocals::GetVisualSupportedEquipmentTypes())
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
			? ResolveEquipmentOverlaySprite(*EquipmentData, CachedAnimationType, CachedAnimationFrameIndex)
			: nullptr;

		OverlayComponent->SetSprite(const_cast<UPaperSprite*>(OverlaySprite));
		const bool bShouldShow = OverlaySprite != nullptr;
		OverlayComponent->SetVisibility(bShouldShow);
		OverlayComponent->SetHiddenInGame(!bShouldShow);
	}
}

void UMKCharacterVisualComponent::UpdateEquipmentOverlayZOrders()
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	const UGameSettingDataAsset* GameSettings = OwnerCharacter->GetGameSettings();
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

void UMKCharacterVisualComponent::UpdateStateSpriteVisuals()
{
	UpdateStateSpriteLocations();
	HideAllStateSprites();

	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return;
	}

	if (CachedAnimationType == ECharacterAnimationType::Drill_Side
		|| CachedAnimationType == ECharacterAnimationType::Drill_Up
		|| CachedAnimationType == ECharacterAnimationType::Drill_Down)
	{
		const FName* EquippedDrillItemKeyPtr = EquippedOverlayItemKeys.Find(EEuipmentType::Drill);
		const FEquipmentItemDataTableRow* DrillEquipmentData = EquippedDrillItemKeyPtr != nullptr
			? GetEquipmentItemData(*EquippedDrillItemKeyPtr)
			: nullptr;

		UPaperSpriteComponent* TargetComponent = nullptr;
		switch (CachedAnimationType)
		{
		case ECharacterAnimationType::Drill_Side:
			TargetComponent = DrillSideStateSpriteComponent.Get();
			break;
		case ECharacterAnimationType::Drill_Up:
			TargetComponent = DrillUpStateSpriteComponent.Get();
			break;
		case ECharacterAnimationType::Drill_Down:
			TargetComponent = DrillDownStateSpriteComponent.Get();
			break;
		default:
			break;
		}

		const UPaperSprite* DrillStateSprite = DrillEquipmentData != nullptr
			? ResolveEquipmentStateSprite(*DrillEquipmentData)
			: nullptr;
		ApplyStateSpriteDisplay(TargetComponent, DrillStateSprite);
		return;
	}

	const UCharacterMovementComponent* MoveComponent = OwnerCharacter->GetCharacterMovement();
	if (::IsValid(MoveComponent) == false || MoveComponent->MovementMode != MOVE_Flying)
	{
		return;
	}

	const FName* EquippedBalloonItemKeyPtr = EquippedOverlayItemKeys.Find(EEuipmentType::Balloon);
	const FEquipmentItemDataTableRow* BalloonEquipmentData = EquippedBalloonItemKeyPtr != nullptr
		? GetEquipmentItemData(*EquippedBalloonItemKeyPtr)
		: nullptr;

	const UPaperSprite* BalloonStateSprite = BalloonEquipmentData != nullptr
		? ResolveEquipmentStateSprite(*BalloonEquipmentData)
		: nullptr;
	ApplyStateSpriteDisplay(BalloonStateSpriteComponent.Get(), BalloonStateSprite);
}

void UMKCharacterVisualComponent::UpdateStateSpriteLocations()
{
	const auto ApplyFacingLocation = [this](UPaperSpriteComponent* StateSpriteComponent, const FVector& LeftFacingLocation)
	{
		if (::IsValid(StateSpriteComponent) == false)
		{
			return;
		}

		FVector UpdatedLocation = LeftFacingLocation;
		if (bFacingRight)
		{
			UpdatedLocation.X *= -1.f;
		}

		StateSpriteComponent->SetRelativeLocation(UpdatedLocation);
	};

	ApplyFacingLocation(BalloonStateSpriteComponent, BalloonLeftFacingRelativeLocation);
	ApplyFacingLocation(DrillSideStateSpriteComponent, DrillSideLeftFacingRelativeLocation);
	ApplyFacingLocation(DrillDownStateSpriteComponent, DrillDownLeftFacingRelativeLocation);
	ApplyFacingLocation(DrillUpStateSpriteComponent, DrillUpLeftFacingRelativeLocation);
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

void UMKCharacterVisualComponent::ApplyStateSpriteDisplay(UPaperSpriteComponent* TargetComponent, const UPaperSprite* TargetSprite)
{
	if (::IsValid(TargetComponent) == false)
	{
		return;
	}

	TargetComponent->SetSprite(const_cast<UPaperSprite*>(TargetSprite));
	const bool bShouldShow = TargetSprite != nullptr;
	TargetComponent->SetVisibility(bShouldShow);
	TargetComponent->SetHiddenInGame(!bShouldShow);
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

UInventoryComponent* UMKCharacterVisualComponent::GetInventoryComponent() const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) ? OwnerCharacter->FindComponentByClass<UInventoryComponent>() : nullptr;
}

UAbilitySystemComponent* UMKCharacterVisualComponent::GetOwnerAbilitySystemComponent() const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
}

int32 UMKCharacterVisualComponent::ResolveStateSpriteSortPriority() const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return 1;
	}

	const UGameSettingDataAsset* GameSettings = OwnerCharacter->GetGameSettings();
	int32 HighestEquipmentZOrder = 0;
	for (const EEuipmentType EquipmentType : MKCharacterVisualComponentLocals::GetVisualSupportedEquipmentTypes())
	{
		HighestEquipmentZOrder = FMath::Max(HighestEquipmentZOrder, ::IsValid(GameSettings) ? GameSettings->GetEquipmentOverlayZOrder(EquipmentType) : 0);
	}

	return HighestEquipmentZOrder + 1;
}

const FEquipmentItemDataTableRow* UMKCharacterVisualComponent::GetEquipmentItemData(FName EquipmentKey) const
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	return ::IsValid(OwnerCharacter) ? FEquipmentItemDataTableUtil::FindEquipmentItemData(OwnerCharacter, EquipmentKey) : nullptr;
}

const UPaperSprite* UMKCharacterVisualComponent::ResolveEquipmentOverlaySprite(const FEquipmentItemDataTableRow& EquipmentData, ECharacterAnimationType AnimationType, int32 AnimationFrameIndex)
{
	AMKCharacter* OwnerCharacter = GetOwnerCharacter();
	if (::IsValid(OwnerCharacter) == false)
	{
		return nullptr;
	}

	const TSoftObjectPtr<UTexture2D>* OverlayTexturePtr = EquipmentData.AnimationOverlayTextures.FindTexture(AnimationType);
	if (OverlayTexturePtr != nullptr && OverlayTexturePtr->IsNull() == false)
	{
		UTexture2D* OverlayAtlasTexture = OverlayTexturePtr->LoadSynchronous();
		return ResolveAnimationAtlasSprite(OverlayAtlasTexture, AnimationFrameIndex, OwnerCharacter->ResolveCurrentBasePixelsPerUnrealUnit());
	}

	if (UTexture2D* IdlePreviewTexture = FEquipmentItemDataTableUtil::LoadIdlePreviewTexture(EquipmentData))
	{
		return ResolveAnimationAtlasSprite(IdlePreviewTexture, 0, OwnerCharacter->ResolveCurrentBasePixelsPerUnrealUnit());
	}

	return nullptr;
}

const UPaperSprite* UMKCharacterVisualComponent::ResolveEquipmentStateSprite(const FEquipmentItemDataTableRow& EquipmentData)
{
	if (EquipmentData.StateDisplaySprite.IsNull())
	{
		return nullptr;
	}

	return EquipmentData.StateDisplaySprite.LoadSynchronous();
}

const UPaperSprite* UMKCharacterVisualComponent::ResolveAnimationAtlasSprite(UTexture2D* AtlasTexture, int32 AnimationFrameIndex, float PixelsPerUnrealUnit)
{
	if (::IsValid(AtlasTexture) == false)
	{
		return nullptr;
	}

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize;
	const int32 AtlasRows = AtlasTexture->GetSizeY() / MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize;
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

	const int32 AtlasColumns = AtlasTexture->GetSizeX() / MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize;
	if (AtlasColumns <= 0)
	{
		return nullptr;
	}

	const FIntPoint CellOrigin(
		(AtlasCellIndex % AtlasColumns) * MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize,
		(AtlasCellIndex / AtlasColumns) * MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize);

	UMKRuntimePaperSprite* RuntimeSprite = NewObject<UMKRuntimePaperSprite>(this);
	if (::IsValid(RuntimeSprite) == false)
	{
		return nullptr;
	}

		RuntimeSprite->InitializeFromAtlasCell(
		AtlasTexture,
		CellOrigin,
		FIntPoint(MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize, MKCharacterVisualComponentLocals::VisualEquipmentOverlayAtlasCellSize),
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
