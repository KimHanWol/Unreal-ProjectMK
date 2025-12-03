// LINK

#include "ProjectMK/Actor/Block/BlockBase.h"

#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"

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

void ABlockBase::InitializeBlock(FBlockTileData InBlockTileData)
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

	BlockTileData = InBlockTileData;

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return;
    }

    TSoftObjectPtr<UPaperSprite> SoftPaperSprite = BlockDataTableRow->TileSprite;

    float SpawnProbabilityValue = FMath::RandRange(0.f, 1.f);
    float CurrntItemSpawnProb = 0.f;
    for (const auto& SpawnableItemData : BlockDataTableRow->SpawnableItemDataList)
    {
        CurrntItemSpawnProb += SpawnableItemData.SpawnProbability;
        if (SpawnProbabilityValue < CurrntItemSpawnProb)
        {
            SoftPaperSprite = SpawnableItemData.TileSprite;
            SpawnableItemKey = SpawnableItemData.SpawnableItemKey;
            break;
        }
    }

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

        FVector2D TileSize = FVector2D(BlockTileData.TileSize.X, BlockTileData.TileSize.Y);
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

        SetActorLocation(BlockTileData.WorldLocation);
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
    if (::IsValid(AbilitySystemComponent))
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
}

const FGameplayTag ABlockBase::GetInteractEventTag()
{
    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return FGameplayTag::EmptyTag;
    }

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return FGameplayTag::EmptyTag;
    }

    return BlockDataTableRow->InteractEventTag;
}

bool ABlockBase::TryInteract(AActor* Interactor)
{
    if (!CanInteract(Interactor))
    {
        return false;
    }

    if (bIsInteracting)
    {
        return false;
    }
    bIsInteracting = true;

    TWeakObjectPtr<ABlockBase> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimer(InteractingTimerHandle, [WeakThis, Interactor]()
        {
            if (WeakThis.IsValid())
            {
                ABlockBase* StrongThis = WeakThis.Get();
                if (::IsValid(StrongThis))
                {
                    StrongThis->TryInteract_Internal(Interactor);
                }
            }
        }, MiningDuration, true, MiningDuration);

    return true;
}

void ABlockBase::EndInteract()
{
    if (!bIsInteracting)
    {
        return;
    }
    bIsInteracting = false;

    GetWorld()->GetTimerManager().ClearTimer(InteractingTimerHandle);
}

void ABlockBase::TryInteract_Internal(AActor* Interactor)
{
    if (!bIsInteracting)
    {
        return;
    }

    IInteractable::TryInteract(Interactor);
}

UAbilitySystemComponent* ABlockBase::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ABlockBase::BeginPlay()
{
    Super::BeginPlay();

    BindEvents();
}

void ABlockBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UnbindEvents();
}

void ABlockBase::OnPreDestroy()
{
    if (SpawnableItemKey.IsNone() == false)
    {
        SpawnItem();
    }
}

void ABlockBase::BindEvents()
{
    if (::IsValid(AbilitySystemComponent) == false)
    {
        return;
    }

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Block::GetDurabilityAttribute()).AddUObject(this, &ABlockBase::OnDurationChanged);
}

void ABlockBase::UnbindEvents()
{
    if (::IsValid(AbilitySystemComponent) == false)
    {
        return;
    }

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Block::GetDurabilityAttribute()).RemoveAll(this);
}

void ABlockBase::OnPaperSpriteLoaded()
{
	InitializeBlock(BlockTileData);
}

void ABlockBase::InitializeBlockAttribute()
{
    if (::IsValid(AbilitySystemComponent) == false)
    {
        return;
    }

    AbilitySystemComponent->AddAttributeSetSubobject(NewObject<UAttributeSet_Block>());

    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> EffectClass = DataManager->GetGameplayEffect(EGameplayEffectType::Block_Init);
    if (::IsValid(EffectClass) == false)
    {
        return;
    }

    FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, AbilitySystemComponent->MakeEffectContext());
    if (SpecHandle.IsValid())
    {
        FGameplayTag DurabilityTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Block.Durability"));
        SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("SetByCaller.Block.Durability")), BlockDataTableRow->BlockDurability);
        AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}

void ABlockBase::OnDurationChanged(const FOnAttributeChangeData& Data)
{
    UDataManager* DataManager = UDataManager::Get(this);
    if (::IsValid(DataManager) == false)
    {
        return;
    }

    const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
    if (BlockDataTableRow == nullptr)
    {
        return;
    }

    if (BlockDataTableRow->bIsMineable == false)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Block duration changed (%f -> %f)"), Data.OldValue, Data.NewValue);

    if (Data.NewValue <= 0)
    {
        OnPreDestroy();
        Destroy();
    }
}

void ABlockBase::SpawnItem()
{
    AItemBase* SpawnedItem = GetWorld()->SpawnActor<AItemBase>();
    if (::IsValid(SpawnedItem) == false)
    {
        return;
    }

    SpawnedItem->InitializeItemBase(SpawnableItemKey);
    SpawnedItem->SetActorLocation(GetActorLocation());
}
