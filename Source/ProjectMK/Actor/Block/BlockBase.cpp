// LINK

#include "ProjectMK/Actor/Block/BlockBase.h"

#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Block.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Spawnable/ItemBase.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameplayEffectDataAsset.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/Interface/Minable.h"
#include "UObject/ConstructorHelpers.h"

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

	DestroyFlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("DestroyFlipbook"));
	DestroyFlipbookComponent->SetupAttachment(RootComponent);
	DestroyFlipbookComponent->SetRelativeLocation(FVector::ZeroVector);
	DestroyFlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DestroyFlipbookComponent->SetLooping(false);
	DestroyFlipbookComponent->SetVisibility(false);
	DestroyFlipbookComponent->SetTranslucentSortPriority(2);

	DamageFlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("DamageFlipbook"));
	DamageFlipbookComponent->SetupAttachment(RootComponent);
	DamageFlipbookComponent->SetRelativeLocation(FVector(0.f, 0.2f, 0.f));
	DamageFlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageFlipbookComponent->SetLooping(false);
	DamageFlipbookComponent->SetVisibility(false);
	DamageFlipbookComponent->SetTranslucentSortPriority(2);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterialFinder(TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterialFinder.Succeeded())
	{
		DamageFlipbookMaterial = TranslucentSpriteMaterialFinder.Object;
		DamageFlipbookComponent->SetMaterial(0, DamageFlipbookMaterial);
	}

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

UAbilitySystemComponent* ABlockBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABlockBase::SetLastDamageInstigatorASC(UAbilitySystemComponent* InInstigatorASC)
{
	LastDamageInstigatorASC = InInstigatorASC;
}

UAbilitySystemComponent* ABlockBase::GetLastDamageInstigatorASC() const
{
	return LastDamageInstigatorASC.Get();
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
	if (bIsDestroying)
	{
		return;
	}

	bIsDestroying = true;
	OnPreDestroy();
	PlayDestroyFlipbook();
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

	MaxDurability = BlockDataTableRow->BlockDurability;

	if (bDropSelectionInitialized == false)
	{
		InitializeDropSelection(*BlockDataTableRow, DataManager);
		bDropSelectionInitialized = true;
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
			FVector BoxExtent = FVector(TileSize.X * 0.5f, 10.f, TileSize.Y * 0.5f);

			BoxCollision->SetBoxExtent(BoxExtent, false);
			BoxCollision->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
			BoxCollision->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			BoxCollision->SetCollisionProfileName(TEXT("NoCollision"));
		}

		SetActorLocation(BlockTileData.WorldLocation);
	}
	else if (SoftPaperSprite.IsNull() == false)
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(
			SoftPaperSprite.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &ABlockBase::OnPaperSpriteLoaded)
		);
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

	UpdateDamageFlipbook();
}

bool ABlockBase::TryAddSpawnItem(FName ItemKey, int32 ItemCount)
{
	if (ItemKey.IsNone() || ItemCount <= 0)
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return false;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemKey);
	if (ItemDataTableRow == nullptr)
	{
		return false;
	}

	for (FSelectedBlockItemData& SelectedSpawnItemData : SelectedSpawnItemDataList)
	{
		if (SelectedSpawnItemData.ItemKey == ItemKey)
		{
			SelectedSpawnItemData.ItemCount += ItemCount;
			return true;
		}
	}

	FSelectedBlockItemData& SelectedSpawnItemData = SelectedSpawnItemDataList.AddDefaulted_GetRef();
	SelectedSpawnItemData.ItemKey = ItemKey;
	SelectedSpawnItemData.ItemCount = ItemCount;
	SelectedSpawnItemData.bShouldSplitDrop = ItemDataTableRow->bIsSplitDrop;
	return true;
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
			FGameplayAbilityUtils::ApplyDamage(WeakBlock.Get()->GetAbilitySystemComponent(), Miner->GetOwnerASC(), MiningDamage);
		}
	}, MiningDuration, true, MiningDuration);
}

void ABlockBase::EndMineBlock()
{
	bIsMining = false;

	GetWorld()->GetTimerManager().ClearTimer(BreakingTimerHandle);
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

void ABlockBase::OnPreDestroy()
{
	BlockTileData.OnBlockDestroyedDelegate.Broadcast(this);
	UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][Block] Destroyed block %s SpawnItemTypeCount=%d InstigatorASC=%s"),
		*GetNameSafe(this),
		SelectedSpawnItemDataList.Num(),
		*GetNameSafe(GetLastDamageInstigatorASC()));

	if (UAbilitySystemComponent* InstigatorAbilitySystemComponent = GetLastDamageInstigatorASC())
	{
		const FGameplayTag BlockDestroyedEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Block.Destroyed"));
		FGameplayEventData EventData;
		EventData.EventTag = BlockDestroyedEventTag;
		EventData.Instigator = InstigatorAbilitySystemComponent->GetAvatarActor();
		EventData.Target = this;
		EventData.OptionalObject = this;
		InstigatorAbilitySystemComponent->HandleGameplayEvent(BlockDestroyedEventTag, &EventData);
		UE_LOG(LogTemp, Warning, TEXT("[SkillDebug][Block] Sent BlockDestroyed event to %s"), *GetNameSafe(InstigatorAbilitySystemComponent->GetAvatarActor()));
	}

	SpawnItems();
}

void ABlockBase::PlayDestroyFlipbook()
{
	if (::IsValid(BoxCollision))
	{
		BoxCollision->SetCollisionProfileName(TEXT("NoCollision"));
	}

	EndMineBlock();

	if (::IsValid(PaperSpriteComponent))
	{
		PaperSpriteComponent->SetVisibility(false);
	}

	if (::IsValid(ItemSpriteComponent))
	{
		ItemSpriteComponent->SetVisibility(false);
	}

	if (::IsValid(DamageFlipbookComponent))
	{
		DamageFlipbookComponent->SetVisibility(false);
	}

	if (::IsValid(DestroyFlipbookComponent) == false)
	{
		DestroyBlockActor();
		return;
	}

	UPaperFlipbook* DestroyFlipbook = DestroyFlipbookComponent->GetFlipbook();
	if (::IsValid(DestroyFlipbook) == false)
	{
		DestroyBlockActor();
		return;
	}

	DestroyFlipbookComponent->OnFinishedPlaying.RemoveAll(this);
	DestroyFlipbookComponent->OnFinishedPlaying.AddDynamic(this, &ABlockBase::OnDestroyFlipbookFinished);
	DestroyFlipbookComponent->SetLooping(false);
	DestroyFlipbookComponent->SetVisibility(true);
	DestroyFlipbookComponent->PlayFromStart();
}

void ABlockBase::DestroyBlockActor()
{
	Destroy();
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

	const FGameplayTag DurabilityTag = FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Block.Durability"));
	FGameplayAbilityUtils::ApplyGameplayEffectToSelf(AbilitySystemComponent, EffectClass, DurabilityTag, BlockDataTableRow->BlockDurability);
}

void ABlockBase::InitializeDropSelection(const FBlockDataTableRow& BlockDataTableRow, UDataManager* DataManager)
{
	SelectedSpawnItemDataList.Reset();
	SelectedBaseTileSprite = BlockDataTableRow.TileSprite;
	SelectedItemOverlaySprite = nullptr;

	const float OreSelectionValue = FMath::FRand();
	float AccumulatedOreSpawnWeight = 0.f;
	bool bHasSelectedOre = false;

	for (const FBlockSpawnableItemData& SpawnableItemData : BlockDataTableRow.SpawnableItemDataList)
	{
		if (SpawnableItemData.SpawnableItemKey.IsNone())
		{
			continue;
		}

		const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, SpawnableItemData.SpawnableItemKey);
		if (ItemDataTableRow == nullptr)
		{
			continue;
		}

		if (ItemDataTableRow->bIsOre)
		{
			if (bHasSelectedOre)
			{
				continue;
			}

			AccumulatedOreSpawnWeight = FMath::Min(1.f, AccumulatedOreSpawnWeight + FMath::Clamp(SpawnableItemData.OreSpawnWeight, 0.f, 1.f));
			if (OreSelectionValue >= AccumulatedOreSpawnWeight)
			{
				continue;
			}

			bHasSelectedOre = true;
		}

		const int32 MinSpawnItemCount = FMath::Max(0, SpawnableItemData.MinSpawnItemCount);
		const int32 MaxSpawnItemCount = FMath::Max(MinSpawnItemCount, SpawnableItemData.MaxSpawnItemCount);
		const int32 SpawnItemCount = FMath::RandRange(MinSpawnItemCount, MaxSpawnItemCount);
		if (SpawnItemCount <= 0)
		{
			continue;
		}

		FSelectedBlockItemData& SelectedSpawnItemData = SelectedSpawnItemDataList.AddDefaulted_GetRef();
		SelectedSpawnItemData.ItemKey = SpawnableItemData.SpawnableItemKey;
		SelectedSpawnItemData.ItemCount = SpawnItemCount;
		SelectedSpawnItemData.bShouldSplitDrop = ItemDataTableRow->bIsSplitDrop;

		if (ItemDataTableRow->bIsOre)
		{
			SelectedItemOverlaySprite = ItemDataTableRow->ItemIcon;
		}
	}
}

void ABlockBase::UpdateDamageFlipbook()
{
	if (::IsValid(DamageFlipbookComponent) == false || MaxDurability <= 0.f)
	{
		return;
	}

	if (::IsValid(DamageFlipbookMaterial))
	{
		DamageFlipbookComponent->SetMaterial(0, DamageFlipbookMaterial);
	}

	UPaperFlipbook* DamageFlipbook = DamageFlipbookComponent->GetFlipbook();
	if (::IsValid(DamageFlipbook) == false)
	{
		DamageFlipbookComponent->SetVisibility(false);
		return;
	}

	const UAttributeSet_Block* BlockAttributeSet = Cast<UAttributeSet_Block>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Block::StaticClass()));
	if (::IsValid(BlockAttributeSet) == false)
	{
		DamageFlipbookComponent->SetVisibility(false);
		return;
	}

	const float CurrentDurability = FMath::Clamp(BlockAttributeSet->GetDurability(), 0.f, MaxDurability);
	const float RemainingDurabilityRatio = CurrentDurability / MaxDurability;
	if (RemainingDurabilityRatio >= 1.f)
	{
		DamageFlipbookComponent->SetVisibility(false);
		return;
	}

	const int32 FlipbookFrameCount = DamageFlipbook->GetNumFrames();
	if (FlipbookFrameCount <= 0)
	{
		DamageFlipbookComponent->SetVisibility(false);
		return;
	}

	const int32 DisplayFrameCount = FMath::Min(FlipbookFrameCount, 12);
	const float DamageRatio = 1.f - RemainingDurabilityRatio;
	const int32 FrameIndex = FMath::Clamp(FMath::FloorToInt(DamageRatio * (DisplayFrameCount - 1)), 0, DisplayFrameCount - 1);

	DamageFlipbookComponent->SetLooping(false);
	DamageFlipbookComponent->SetVisibility(true);
	DamageFlipbookComponent->SetSpriteColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.85f));
	DamageFlipbookComponent->Stop();
	DamageFlipbookComponent->SetPlaybackPositionInFrames(FrameIndex, false);
}

void ABlockBase::SpawnItems()
{
	int32 VisualLayerIndex = 1;
	for (const FSelectedBlockItemData& SelectedSpawnItemData : SelectedSpawnItemDataList)
	{
		const int32 AdjustedSpawnItemCount = FGameplayAbilityUtils::CalculateAdjustedItemCount(
			this,
			GetLastDamageInstigatorASC(),
			SelectedSpawnItemData.ItemKey,
			SelectedSpawnItemData.ItemCount
		);
		if (AdjustedSpawnItemCount <= 0)
		{
			continue;
		}

		const int32 SpawnActorCount = SelectedSpawnItemData.bShouldSplitDrop ? AdjustedSpawnItemCount : 1;
		const int32 ItemCountPerActor = SelectedSpawnItemData.bShouldSplitDrop ? 1 : AdjustedSpawnItemCount;
		const bool bUseSpawnOffset = SelectedSpawnItemDataList.Num() > 1 || SpawnActorCount > 1;
		for (int32 SpawnIndex = 0; SpawnIndex < SpawnActorCount; ++SpawnIndex)
		{
			AItemBase* SpawnedItem = GetWorld()->SpawnActor<AItemBase>();
			if (::IsValid(SpawnedItem) == false)
			{
				continue;
			}

			SpawnedItem->InitializeItemBase(SelectedSpawnItemData.ItemKey, ItemCountPerActor);

			const float HorizontalOffset = bUseSpawnOffset ? FMath::FRandRange(-8.f, 8.f) : 0.f;
			const float VerticalOffset = bUseSpawnOffset ? FMath::FRandRange(-6.f, 6.f) : 0.f;
			SpawnedItem->SetActorLocation(GetActorLocation() + FVector(HorizontalOffset, 0.f, VerticalOffset));
			SpawnedItem->SetVisualLayer(static_cast<float>(VisualLayerIndex) * 0.1f, VisualLayerIndex);
			++VisualLayerIndex;
		}
	}
}

void ABlockBase::OnDestroyFlipbookFinished()
{
	if (::IsValid(DestroyFlipbookComponent))
	{
		DestroyFlipbookComponent->OnFinishedPlaying.RemoveAll(this);
	}

	DestroyBlockActor();
}

void ABlockBase::OnDurationChanged(const FOnAttributeChangeData& Data)
{
	if (CheckIsDestroyed())
	{
		OnDestroyed();
		return;
	}

	UpdateDamageFlipbook();
}
