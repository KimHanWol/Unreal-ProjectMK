// LINK

#include "ProjectMK/Actor/Spawnable/ItemBase.h"

#include "Components/SphereComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/MKRuntimePaperSprite.h"
#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"

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
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePosition();
}

void AItemBase::InitializeItemBase(FName InItemKey)
{
	ItemKey = InItemKey;

	if (::IsValid(PaperSpriteComponent) == false)
	{
		return;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	RuntimeItemPreviewSprite = nullptr;

	UPaperSprite* ResolvedItemSprite = nullptr;
	if (const FEquipmentItemDataTableRow* EquipmentData = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, InItemKey))
	{
		RuntimeItemPreviewSprite = Cast<UMKRuntimePaperSprite>(FEquipmentItemDataTableUtil::CreateIdlePreviewSprite(this, *EquipmentData));
		if (::IsValid(RuntimeItemPreviewSprite))
		{
			ResolvedItemSprite = RuntimeItemPreviewSprite;
		}
	}

	if (::IsValid(ResolvedItemSprite) == false)
	{
		const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, InItemKey);
		if (ItemDataTableRow == nullptr)
		{
			return;
		}

		ResolvedItemSprite = ItemDataTableRow->ItemIcon.LoadSynchronous();
	}

	PaperSpriteComponent->SetSprite(ResolvedItemSprite);

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
}

void AItemBase::OnLootFinished()
{
	Destroy();
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
		FVector ItemLocation = GetActorLocation();
		FVector PlayerLocation = Looter->GetActorLocation();

		CurrentLootingSpeed += GetWorld()->GetDeltaSeconds() * LootingAccelerationRate;

		FVector NewLocation = FMath::VInterpConstantTo(ItemLocation, PlayerLocation, GetWorld()->GetDeltaSeconds(), CurrentLootingSpeed);
		SetActorLocation(NewLocation);
	}
	else
	{
		float Time = GetWorld()->GetTimeSeconds();
		float OffsetZ = FMath::Sin(Time * FloatSpeed) * FloatAmplitude;

		PaperSpriteComponent->SetRelativeLocation(FVector(0.f, 0.f, OffsetZ));
	}
}
