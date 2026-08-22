// LINK

#include "ProjectMK/Actor/Spawnable/ItemBase.h"

#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
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
	PaperSpriteComponent->SetRelativeLocation(FVector(0.f, -1.f, 0.f));
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

	UpdatePosition(DeltaTime);
}

void AItemBase::InitializeItemBase(FName InItemKey, int32 InItemCount)
{
	ItemKey = InItemKey;
	ItemCount = FMath::Max(1, InItemCount);
	FloatPhase = FMath::FRandRange(0.f, UE_TWO_PI);

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

	float ItemSpriteScale = 0.7f;
	if (const UGameSettingDataAsset* GameSettings = DataManager->GetGameSettingDataAsset())
	{
		ItemSpriteScale = GameSettings->WorldItemSpriteScale;
	}

	PaperSpriteComponent->SetRelativeScale3D(FVector(ItemSpriteScale, 1.f, ItemSpriteScale));
	bIsInitialized = true;
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
	if (IsOccupied() || InLooter.IsValid() == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = InLooter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false || InventoryComponent->IsItemCollectionActive() == false)
	{
		return;
	}

	Looter = InLooter;
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
	Destroy();
}

void AItemBase::SetVisualLayer(float InVisualYOffset)
{
	VisualYOffset = InVisualYOffset;

	if (::IsValid(PaperSpriteComponent))
	{
		PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, 0.f));
		PaperSpriteComponent->SetTranslucentSortPriority(static_cast<int32>(GetUniqueID()));
	}
}

void AItemBase::ClearLooting()
{
	Looter = nullptr;
	CurrentLootingSpeed = InitialLootingSpeed;
	LootLateralDirection = FVector::ZeroVector;
	LootLateralOffsetScale = 0.f;
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
		const FVector CurvedTargetLocation = PlayerLocation + (LootLateralDirection * LootLateralOffsetScale * LateralOffsetAlpha);
		FVector NewLocation = FMath::VInterpConstantTo(ItemLocation, CurvedTargetLocation, GetWorld()->GetDeltaSeconds(), CurrentLootingSpeed);
		SetActorLocation(NewLocation);

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
		FallingVelocity = FMath::Max(FallingVelocity + (GetWorld()->GetGravityZ() * DeltaTime), -MaxFallingSpeed);

		FHitResult FallingHit;
		AddActorWorldOffset(FVector(0.f, 0.f, FallingVelocity * DeltaTime), true, &FallingHit);
		if (FallingHit.bBlockingHit && FallingVelocity < 0.f)
		{
			FallingVelocity = 0.f;
		}

		const float Time = GetWorld()->GetTimeSeconds();
		const float FloatAlpha = FMath::Sin((Time * FloatSpeed) + FloatPhase) + 1.f;
		const float OffsetZ = FloatAlpha * FloatAmplitude;

		PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, VisualYOffset, OffsetZ));
	}
}
