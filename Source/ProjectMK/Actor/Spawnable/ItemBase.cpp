// LINK

#include "ProjectMK/Actor/Spawnable/ItemBase.h"

#include "Components/SphereComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootComponent = SphereCollision;

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector(0.f, -1.f, -4.f));
	PaperSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PaperSpriteComponent->SetTranslucentSortPriority(1);
	BaseSpriteRelativeLocation = PaperSpriteComponent->GetRelativeLocation();
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePosition();
}

void AItemBase::InitializeItemBase(FName InItemKey, int32 InItemCount)
{
	ItemKey = InItemKey;
	ItemCount = FMath::Max(1, InItemCount);

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

	FTimerHandle InitTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(InitTimerHandle, [this]()
		{
			bIsInitialized = true;
		},
		0.5f, false);
}

bool AItemBase::IsOccupied()
{
	return Looter.IsValid();
}

void AItemBase::TryLoot(TWeakObjectPtr<AMKCharacter> InLooter)
{
	if (IsOccupied())
	{
		return;
	}

	Looter = InLooter;
	CurrentLootingSpeed = InitialLootingSpeed;

	if (Looter.IsValid() == false)
	{
		return;
	}

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

void AItemBase::SetVisualLayer(float InActorYOffset, int32 InSortPriority)
{
	ActorYOffset = InActorYOffset;

	FVector NewActorLocation = GetActorLocation();
	NewActorLocation.Y += ActorYOffset;
	SetActorLocation(NewActorLocation);

	if (::IsValid(PaperSpriteComponent))
	{
		PaperSpriteComponent->SetTranslucentSortPriority(InSortPriority);
	}
}

void AItemBase::ClearLooting()
{
	Looter = nullptr;
	CurrentLootingSpeed = InitialLootingSpeed;
	LootLateralDirection = FVector::ZeroVector;
	LootLateralOffsetScale = 0.f;
}

void AItemBase::UpdatePosition()
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
			if (UInventoryComponent* InventoryComponent = Looter->GetComponentByClass<UInventoryComponent>())
			{
				if (InventoryComponent->TryCollectWorldItem(this) == false)
				{
					ClearLooting();
				}
			}
			else
			{
				ClearLooting();
			}
		}
	}
	else
	{
		float Time = GetWorld()->GetTimeSeconds();
		float OffsetZ = FMath::Sin(Time * FloatSpeed) * FloatAmplitude;

		PaperSpriteComponent->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(0.f, 0.f, OffsetZ));
	}
}
