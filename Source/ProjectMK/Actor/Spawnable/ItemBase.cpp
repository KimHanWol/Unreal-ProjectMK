// LINK

#include "ProjectMK/Actor/Spawnable/ItemBase.h"

#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->InitSphereRadius(4.f);
	SphereCollision->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision->SetGenerateOverlapEvents(true);
	RootComponent = SphereCollision;

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector(0.f, 24.f, 0.f));
	PaperSpriteComponent->SetTranslucentSortPriority(10);
	PaperSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseSpriteRelativeLocation = PaperSpriteComponent->GetRelativeLocation();

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterialFinder(TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterialFinder.Succeeded())
	{
		PaperSpriteComponent->SetMaterial(0, TranslucentSpriteMaterialFinder.Object);
	}
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DropOwner.IsValid())
	{
		const UInventoryComponent* Inventory = DropOwner->FindComponentByClass<UInventoryComponent>();
		const float Range = ::IsValid(Inventory) ? Inventory->GetItemGainRange() + GetCollisionRadius() : 0.f;
		if (FVector::DistSquared(GetActorLocation(), DropOwner->GetActorLocation()) > FMath::Square(Range)) { DropOwner.Reset(); }
	}
	if (::IsValid(PaperSpriteComponent) && GetLifeSpan() > 0.f && GetLifeSpan() <= BlinkDuration)
	{
		const bool bVisible = FMath::Fmod(GetLifeSpan(), 0.4f) >= 0.2f;
		PaperSpriteComponent->SetVisibility(ItemKey != TEXT("Oxygen") && bVisible);
		if (::IsValid(OxygenGlow)) { OxygenGlow->SetVisibility(bVisible); }
		if (::IsValid(OxygenCore)) { OxygenCore->SetVisibility(bVisible); }
	}
	UpdatePosition(DeltaTime);
}

void AItemBase::InitializeItemBase(FName InItemKey, int32 InItemCount)
{
	ItemKey = InItemKey;
	ItemCount = FMath::Max(1, InItemCount);
	FloatElapsedTime = 0.f;

	if (::IsValid(PaperSpriteComponent) == false)
	{
		return;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, InItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return;
	}

	UPaperSprite* LoadedItemSprite = ItemDataTableRow->ItemIcon.LoadSynchronous();
	PaperSpriteComponent->SetSprite(LoadedItemSprite);
	PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f));

	float DisplaySize = 10.f;
	float Lifetime = 300.f;
	if (const UGameSettingDataAsset* GameSettings = DataManager->GetGameSettingDataAsset())
	{
		DisplaySize = GameSettings->WorldItemDisplaySize * (InItemKey == TEXT("Coin") ? GameSettings->WorldCoinSizeMultiplier : 1.f);
		Lifetime = GameSettings->WorldItemLifetime;
		BlinkDuration = GameSettings->WorldItemBlinkDuration;
	}

	const FVector2D SourceSize = ::IsValid(LoadedItemSprite) ? LoadedItemSprite->GetSourceSize() : FVector2D(32.f);
	const float PixelsPerUnit = ::IsValid(LoadedItemSprite) ? LoadedItemSprite->GetPixelsPerUnrealUnit() : 2.f;
	const float ItemSpriteScale = DisplaySize * PixelsPerUnit / FMath::Max(1.f, static_cast<float>(FMath::Max(SourceSize.X, SourceSize.Y)));
	PaperSpriteComponent->SetRelativeScale3D(FVector(ItemSpriteScale, 1.f, ItemSpriteScale));
	if (ItemKey == TEXT("Oxygen")) { InitializeOxygenVisuals(); }
	bIsInitialized = true;
	SetLifeSpan(FMath::Max(1.f, Lifetime));
}

float AItemBase::GetCollisionRadius() const
{
	if (::IsValid(SphereCollision) == false)
	{
		return 0.f;
	}

	return SphereCollision->GetScaledSphereRadius();
}

bool AItemBase::IsOccupied()
{
	return Looter.IsValid();
}

void AItemBase::TryLoot(TWeakObjectPtr<AMKCharacter> InLooter)
{
	if (IsOccupied() || InLooter.IsValid() == false || CanBeCollectedBy(InLooter.Get()) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = InLooter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false || InventoryComponent->IsItemCollectionActive() == false)
	{
		return;
	}

	Looter = InLooter;
	OxygenTrailPositions.Reset();
	CurrentLootingSpeed = InitialLootingSpeed;
	FallingVelocity = 0.f;

	FVector BaseDirection = Looter->GetCharacterDirection();
	BaseDirection.Y = 0.f;
	if (BaseDirection.IsNearlyZero())
	{
		BaseDirection = Looter->GetActorLocation() - GetActorLocation();
		BaseDirection.Y = 0.f;
	}

	BaseDirection = BaseDirection.GetSafeNormal();
	if (BaseDirection.IsNearlyZero())
	{
		BaseDirection = FVector::ForwardVector;
	}

	LootLateralDirection = FVector(-BaseDirection.Z, 0.f, BaseDirection.X).GetSafeNormal();
	if (LootLateralDirection.IsNearlyZero())
	{
		LootLateralDirection = FVector::UpVector;
	}

	const float SideSign = FMath::RandBool() ? 1.f : -1.f;
	LootLateralOffsetScale = FMath::FRandRange(12.f, 36.f) * SideSign;
}

void AItemBase::OnLootFinished()
{
	if (ItemKey != TEXT("Oxygen")) { UMKBlueprintFunctionLibrary::PlayVisualEffect(this, TEXT("Pickup"), GetActorLocation(), 0.6f); }
	Destroy();
}

void AItemBase::SetVisualLayer(float InVisualYOffset)
{
	VisualYOffset = FMath::Clamp(InVisualYOffset, 0.f, 4.f);

	if (::IsValid(PaperSpriteComponent))
	{
		PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f));
		PaperSpriteComponent->SetTranslucentSortPriority(10);
	}
	if (::IsValid(OxygenGlow)) { OxygenGlow->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f)); }
	if (::IsValid(OxygenCore)) { OxygenCore->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f)); }
}

void AItemBase::SetDropOwner(AMKCharacter* Character)
{
	ClearLooting();
	DropOwner = Character;
}

bool AItemBase::CanBeCollectedBy(const AActor* Character) const
{
	return DropOwner.IsValid() == false || DropOwner.Get() != Character;
}

void AItemBase::InitializeOxygenVisuals()
{
	const UDataManager* Data = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
	const UGameSettingDataAsset* Settings = ::IsValid(Data) ? Data->GetGameSettingDataAsset() : nullptr;
	UPaperFlipbook* Effect = ::IsValid(Style) ? Style->Effects.FindRef(TEXT("Pickup")) : nullptr;
	if (::IsValid(Effect) == false) { return; }
	PaperSpriteComponent->SetVisibility(false);
	// The side-view camera faces -Y; positive Y puts oxygen ahead of depth-writing terrain.
	BaseSpriteRelativeLocation.Y = 30.f;
	OxygenArcHeight = ::IsValid(Settings) ? Settings->OxygenPickupArcHeight : 14.f;
	const float Size = ::IsValid(Settings) ? Settings->OxygenPickupDisplaySize : 8.f;
	const UPaperSprite* Frame = Effect->GetSpriteAtFrame(1);
	const float Scale = ::IsValid(Frame) ? Size * Frame->GetPixelsPerUnrealUnit() / FMath::Max(1.f, static_cast<float>(Frame->GetSourceSize().GetMax())) : 1.f;
	OxygenGlow = NewObject<UPaperFlipbookComponent>(this);
	OxygenGlow->SetupAttachment(RootComponent);
	OxygenGlow->SetFlipbook(Effect);
	OxygenGlow->SetLooping(true);
	OxygenGlow->SetSpriteColor(FLinearColor(0.35f, 0.8f, 0.95f, 0.95f));
	OxygenGlow->SetRelativeLocation(BaseSpriteRelativeLocation);
	OxygenGlow->SetRelativeScale3D(FVector(Scale * 0.4f, 1.f, Scale * 0.4f));
	OxygenGlow->SetSpriteColor(FLinearColor(0.35f, 0.8f, 0.95f, 0.18f));
	OxygenGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OxygenGlow->SetTranslucentSortPriority(12);
	if (::IsValid(Style->OxygenPickupMaterial)) { OxygenGlow->SetMaterial(0, Style->OxygenPickupMaterial); }
	OxygenGlow->RegisterComponent();
	OxygenGlow->Play();
	if (::IsValid(Style->OxygenPickupCore))
	{
		OxygenCore = NewObject<UPaperSpriteComponent>(this);
		OxygenCore->SetupAttachment(RootComponent);
		OxygenCore->SetSprite(Style->OxygenPickupCore);
		const UPaperSprite* Core = Style->OxygenPickupCore;
		const float CoreScale = Size * 0.75f * Core->GetPixelsPerUnrealUnit() / FMath::Max(1.f, static_cast<float>(Core->GetSourceSize().GetMax()));
		OxygenCore->SetRelativeScale3D(FVector(CoreScale, 1.f, CoreScale));
		OxygenCore->SetRelativeLocation(BaseSpriteRelativeLocation);
		OxygenCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OxygenCore->SetTranslucentSortPriority(13);
		OxygenCore->RegisterComponent();
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UPaperSpriteComponent* Trail = NewObject<UPaperSpriteComponent>(this);
		Trail->SetupAttachment(RootComponent);
		Trail->SetSprite(::IsValid(Style->OxygenPickupCore) ? Style->OxygenPickupCore.Get() : Effect->GetSpriteAtFrame(1));
		const UPaperSprite* TrailSprite = Trail->GetSprite();
		const float TrailScale = Size * (0.5f - Index * 0.12f) * TrailSprite->GetPixelsPerUnrealUnit() / FMath::Max(1.f, static_cast<float>(TrailSprite->GetSourceSize().GetMax()));
		Trail->SetRelativeScale3D(FVector(TrailScale, 1.f, TrailScale));
		Trail->SetSpriteColor(FLinearColor(0.32f, 0.72f, 0.88f, 0.6f - Index * 0.15f));
		Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Trail->SetTranslucentSortPriority(11);
		if (::IsValid(Style->OxygenPickupCore) == false && ::IsValid(Style->OxygenPickupMaterial)) { Trail->SetMaterial(0, Style->OxygenPickupMaterial); }
		Trail->SetVisibility(false);
		Trail->RegisterComponent();
		OxygenTrail.Add(Trail);
	}
}

void AItemBase::UpdateOxygenTrail()
{
	const FVector Head = GetActorLocation() + BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f);
	if (OxygenTrailPositions.IsEmpty()) { OxygenTrailPositions.Init(Head, 8); }
	OxygenTrailPositions.Insert(Head, 0);
	OxygenTrailPositions.SetNum(8);
	for (int32 Index = 0; Index < OxygenTrail.Num(); ++Index)
	{
		const int32 Sample = (Index + 1) * 2;
		OxygenTrail[Index]->SetVisibility(FVector::DistSquared(Head, OxygenTrailPositions[Sample]) > 0.25f);
		OxygenTrail[Index]->SetWorldLocation(OxygenTrailPositions[Sample]);
	}
}

void AItemBase::ClearLooting()
{
	Looter = nullptr;
	CurrentLootingSpeed = InitialLootingSpeed;
	LootLateralDirection = FVector::ZeroVector;
	LootLateralOffsetScale = 0.f;
	for (UPaperSpriteComponent* Trail : OxygenTrail) { Trail->SetVisibility(false); }
}

void AItemBase::UpdatePosition(float DeltaTime)
{
	if (bIsInitialized == false)
	{
		return;
	}

	if (::IsValid(PaperSpriteComponent) == false)
	{
		return;
	}

	if (IsOccupied())
	{
		if (Looter.IsValid() == false)
		{
			ClearLooting();
			return;
		}

		UInventoryComponent* InventoryComponent = Looter->GetComponentByClass<UInventoryComponent>();
		if (::IsValid(InventoryComponent) == false || InventoryComponent->IsItemCollectionActive() == false)
		{
			ClearLooting();
			return;
		}

		FVector ItemLocation = GetActorLocation();
		FVector PlayerLocation = Looter->GetActorLocation();
		CurrentLootingSpeed += GetWorld()->GetDeltaSeconds() * LootingAccelerationRate;

		const FVector ToPlayer = PlayerLocation - ItemLocation;
		const float DistanceToPlayer = ToPlayer.Size();
		const float LateralOffsetAlpha = FMath::Clamp(DistanceToPlayer / 140.f, 0.f, 1.f);
		FVector Curve = LootLateralDirection * LootLateralOffsetScale * LateralOffsetAlpha;
		if (ItemKey == TEXT("Oxygen"))
		{
			FVector Perpendicular(-ToPlayer.Z, 0.f, ToPlayer.X);
			if (Perpendicular.Z < 0.f) { Perpendicular *= -1.f; }
			Curve = Perpendicular.GetSafeNormal() * FMath::Min(OxygenArcHeight, DistanceToPlayer * 0.35f) * FMath::Clamp(DistanceToPlayer / 40.f, 0.f, 1.f);
		}
		const FVector CurvedTargetLocation = PlayerLocation + Curve;
		FVector NewLocation = FMath::VInterpConstantTo(ItemLocation, CurvedTargetLocation, GetWorld()->GetDeltaSeconds(), CurrentLootingSpeed);
		SetActorLocation(NewLocation);
		if (ItemKey == TEXT("Oxygen")) { UpdateOxygenTrail(); }

		if (FVector::DistSquared(NewLocation, PlayerLocation) <= FMath::Square(4.f))
		{
			if (InventoryComponent->TryCollectWorldItem(this) == false)
			{
				ClearLooting();
			}
		}
	}
	else
	{
		if (ItemKey == TEXT("Oxygen"))
		{
			FloatElapsedTime += DeltaTime;
			if (::IsValid(OxygenGlow))
			{
				OxygenGlow->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, FMath::Sin(FloatElapsedTime * 3.f) * 2.f));
			}
			if (::IsValid(OxygenCore)) { OxygenCore->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, FMath::Sin(FloatElapsedTime * 3.f) * 2.f)); }
			return;
		}
		FallingVelocity = FMath::Max(FallingVelocity + (GetWorld()->GetGravityZ() * DeltaTime), -MaxFallingSpeed);

		FHitResult FallingHit;
		AddActorWorldOffset(FVector(0.f, 0.f, FallingVelocity * DeltaTime), true, &FallingHit);
		if (FallingHit.bBlockingHit && FallingVelocity < 0.f)
		{
			FallingVelocity = 0.f;
		}

		FloatElapsedTime += DeltaTime;
		const float FloatAlpha = 1.f - FMath::Cos(FloatElapsedTime * FloatSpeed);
		const float OffsetZ = FloatAlpha * FloatAmplitude;

		PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, OffsetZ));
	}
}
