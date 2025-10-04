// LINK

#include "ProjectMK/Actor/Block/BlockBase.h"

#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "AbilitySystemComponent.h"

ABlockBase::ABlockBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector::ZeroVector);

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

void ABlockBase::InitializeBlock(FBlockData InBlockData)
{
	if (::IsValid(BoxCollision) == false)
	{
		return;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	if (::IsValid(PaperSpriteComponent) == false)
	{
		return;
	}

	BlockData = InBlockData;

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return;
    }

    TSoftObjectPtr<UPaperSprite> SoftPaperSprite = BlockDataTableRow->TileSprite;
    if (SoftPaperSprite.IsNull())
    {
        return;
    }

    if (SoftPaperSprite.IsValid())
    {
        UPaperSprite* Sprite = SoftPaperSprite.LoadSynchronous();
        if (::IsValid(Sprite) == false)
        {
            return;
        }

        PaperSpriteComponent->SetSprite(Sprite);

        FVector2D TileSize = FVector2D(BlockData.TileSize.X, BlockData.TileSize.Y);
        FVector2D SpriteSize = Sprite->GetSourceSize();

        float PixelsPerUnit = Sprite->GetPixelsPerUnrealUnit();
        FVector2D WorldSpriteSize = SpriteSize / PixelsPerUnit;

        FVector InSpriteScale = FVector(
            TileSize.X / WorldSpriteSize.X,
            TileSize.Y / WorldSpriteSize.Y,
            1.0f
        );

        PaperSpriteComponent->SetRelativeScale3D(InSpriteScale);

        if (BlockDataTableRow->bHasCollision)
        {
            FVector BoxExtent = FVector(
                TileSize.X * 0.5f,
                10.f,
                TileSize.Y * 0.5f
            );

            BoxCollision->SetBoxExtent(BoxExtent, false); // false: 스케일 적용 안 함
            BoxCollision->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
            BoxCollision->SetCollisionProfileName(TEXT("BlockAll"));
        }
        else
        {
            BoxCollision->SetCollisionProfileName(TEXT("NoCollision"));
        }

        SetActorLocation(BlockData.WorldLocation);
    }
    else
    {
        // 비동기 로딩 처리
        if (!SoftPaperSprite.IsNull())
        {
            FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
            Streamable.RequestAsyncLoad(
                SoftPaperSprite.ToSoftObjectPath(),
                FStreamableDelegate::CreateUObject(this, &ABlockBase::OnPaperSpriteLoaded)
            );
        }
    }

    InitializeBlockAttribute();
}

bool ABlockBase::Interact(AActor* Caller)
{
    return true;
}

UAbilitySystemComponent* ABlockBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ABlockBase::OnPaperSpriteLoaded()
{
	InitializeBlock(BlockData);
}

void ABlockBase::InitializeBlockAttribute()
{
    if (::IsValid(AbilitySystemComponent) == false)
    {
        return;
    }

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return;
    }

    //FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(BlockDataTableRow->BlockInitEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
    //AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
