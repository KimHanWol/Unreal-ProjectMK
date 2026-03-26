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
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"
#include "ProjectMK/Interface/Minable.h"

ABlockBase::ABlockBase()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;

	PaperSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("PaperSprite"));
	PaperSpriteComponent->SetupAttachment(RootComponent);
	PaperSpriteComponent->SetRelativeLocation(FVector::ZeroVector);
	PaperSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ItemSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("ItemSprite"));
	ItemSpriteComponent->SetupAttachment(RootComponent);
	ItemSpriteComponent->SetRelativeLocation(FVector(0.f, 0.1f, 0.f));
	ItemSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemSpriteComponent->SetTranslucentSortPriority(1);

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

	if (::IsValid(PaperSpriteComponent) == false || ::IsValid(ItemSpriteComponent) == false)
	{
		return;
	}

	BlockTileData = InBlockTileData;

	const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
	if (BlockDataTableRow == nullptr)
	{
		return;
	}

	if (bVisualSelectionInitialized == false)
	{
		SpawnableItemKey = NAME_None;
		SelectedBaseTileSprite = BlockDataTableRow->TileSprite;
		SelectedItemOverlaySprite = nullptr;

		float SpawnProbabilityValue = FMath::RandRange(0.f, 1.f);
		float CurrntItemSpawnProb = 0.f;
		for (const auto& SpawnableItemData : BlockDataTableRow->SpawnableItemDataList)
		{
			CurrntItemSpawnProb += SpawnableItemData.SpawnProbability;
			if (SpawnProbabilityValue < CurrntItemSpawnProb)
			{
				SpawnableItemKey = SpawnableItemData.SpawnableItemKey;
				SelectedItemOverlaySprite = SpawnableItemData.ItemSprite;
				break;
			}
		}

		bVisualSelectionInitialized = true;
	}

	TSoftObjectPtr<UPaperSprite> SoftPaperSprite = SelectedBaseTileSprite;

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

		const bool bNeedToBeHide = BlockDataTableRow->bNeedTobeHide;
		if (bNeedToBeHide == false)
		{
			PaperSpriteComponent->SetSprite(Sprite);
			ApplySpriteToComponent(PaperSpriteComponent, Sprite, BlockTileData.TileSize);
		}
		else
		{
			PaperSpriteComponent->SetSprite(nullptr);
		}

		PaperSpriteComponent->SetVisibility(bNeedToBeHide == false);

		if (bNeedToBeHide == false && SelectedItemOverlaySprite.IsNull() == false)
		{
			UPaperSprite* OverlaySprite = SelectedItemOverlaySprite.LoadSynchronous();
			float OverlaySpriteScale = 0.7f;
			if (const UGameSettingDataAsset* GameSettings = DataManager->GetGameSettingDataAsset())
			{
				OverlaySpriteScale = GameSettings->BlockItemOverlaySpriteScale;
			}

			ItemSpriteComponent->SetSprite(OverlaySprite);
			ItemSpriteComponent->SetVisibility(::IsValid(OverlaySprite));
			if (::IsValid(OverlaySprite))
			{
				ApplySpriteToComponent(ItemSpriteComponent, OverlaySprite, BlockTileData.TileSize, OverlaySpriteScale);
			}
		}
		else
		{
			ItemSpriteComponent->SetSprite(nullptr);
			ItemSpriteComponent->SetVisibility(false);
		}

		if (BlockDataTableRow->bHasCollision)
		{
			const FVector2D TileSize = FVector2D(BlockTileData.TileSize.X, BlockTileData.TileSize.Y);
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

	if (bBlockAttributesInitialized == false)
	{
		InitializeBlockAttribute();
		bBlockAttributesInitialized = true;
	}

	if (::IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ABlockBase::StartMineBlock(IMinable* Miner)
{
	if (bIsMining)
	{
		return;
	}
	bIsMining = true;

	float MiningDamage = Miner->GetMiningDamage();
	float MiningDuration = Miner->GetMiningDuration();

	TWeakObjectPtr<ABlockBase> WeakBlock(this);
	GetWorld()->GetTimerManager().ClearTimer(BreakingTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(BreakingTimerHandle, [WeakBlock, Miner, MiningDamage]()
	{
		if (WeakBlock.IsValid() && Miner == nullptr)
		{
			FDamageableUtil::ApplyDamage(WeakBlock.Get()->GetAbilitySystemComponent(), Miner->GetOwnerASC(), MiningDamage);
		}
	}, MiningDuration, true, MiningDuration);
}

void ABlockBase::EndMineBlock()
{
	bIsMining = false;

	GetWorld()->GetTimerManager().ClearTimer(BreakingTimerHandle);
}

UAbilitySystemComponent* ABlockBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABlockBase::SetMineableState(bool bInIsMineableState)
{
	bIsMineableState = bInIsMineableState;
}

bool ABlockBase::IsMineable()
{
	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return true;
	}

	const FBlockDataTableRow* BlockDataTableRow = DataManager->GetBlockDataTableRow(BlockTileData.TileSetIndex);
	if (BlockDataTableRow == nullptr)
	{
		return true;
	}

	return BlockDataTableRow->bIsMineable && bIsMineableState;
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

	BlockTileData.OnBlockDestroyedDelegate.Broadcast(this);
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

UAbilitySystemComponent* ABlockBase::GetOwnerASC()
{
	return GetAbilitySystemComponent();
}

bool ABlockBase::CheckIsDestroyed()
{
	if (::IsValid(AbilitySystemComponent) == false)
	{
		return false;
	}

	const UAttributeSet_Block* BlockAttributeSet = Cast<UAttributeSet_Block>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()));
	if (::IsValid(BlockAttributeSet) == false)
	{
		return false;
	}

	return BlockAttributeSet->GetDurability() <= 0.f;
}

void ABlockBase::OnDestroyed()
{
	OnPreDestroy();
	Destroy();
}

void ABlockBase::OnPaperSpriteLoaded()
{
	InitializeBlock(BlockTileData);
}

void ABlockBase::ApplySpriteToComponent(UPaperSpriteComponent* SpriteComponent, UPaperSprite* Sprite, const FIntPoint& TileSize, float ScaleMultiplier)
{
	if (::IsValid(SpriteComponent) == false || ::IsValid(Sprite) == false)
	{
		return;
	}

	const FVector2D TileSize2D = FVector2D(TileSize.X, TileSize.Y);
	const FVector2D SpriteSize = Sprite->GetSourceSize();
	const float PixelsPerUnit = Sprite->GetPixelsPerUnrealUnit();
	const FVector2D WorldSpriteSize = SpriteSize / PixelsPerUnit;

	const FVector InSpriteScale = FVector(
		(TileSize2D.X / WorldSpriteSize.X) * ScaleMultiplier,
		1.0f,
		(TileSize2D.Y / WorldSpriteSize.Y) * ScaleMultiplier
	);

	SpriteComponent->SetRelativeScale3D(InSpriteScale);
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
		SpecHandle.Data->SetSetByCallerMagnitude(DurabilityTag, BlockDataTableRow->BlockDurability);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ABlockBase::OnDurationChanged(const FOnAttributeChangeData& Data)
{
	if (CheckIsDestroyed())
	{
		OnDestroyed();
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
