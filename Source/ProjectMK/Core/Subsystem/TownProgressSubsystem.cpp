#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/MineFacilityActor.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/MKSaveGame.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

void UTownProgressSubsystem::Tick(float DeltaSeconds)
{
	if (GetWorld()->IsGameWorld() == false) { return; }
	UpdateTime += DeltaSeconds;
	if (UpdateTime < 0.5f) { return; }
	UpdateTime = 0.f;
	AMKCharacter* Player = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (Player == nullptr) { return; }
	LoadProgress();
	const int32 Surface = UDataManager::Get(this)->GetGameSettingDataAsset()->SurfaceBlockPositionY;
	const int32 Depth = FMath::Max(0, FMath::FloorToInt(UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(Player->GetActorLocation()).Y) - Surface);
	if (Depth > MaximumDepth) { MaximumDepth = Depth; bProgressDirty = true; OnTownProgressChanged.Broadcast(); }
	UMineTransportSubsystem* Rail = GetWorld()->GetSubsystem<UMineTransportSubsystem>();
	if ((bProgressDirty || Rail->IsDirty()) && GetWorld()->GetTimeSeconds() - LastSavedAt >= 2.f)
	{
		if (Player->TrySaveGame()) { bProgressDirty = false; Rail->ClearDirty(); LastSavedAt = GetWorld()->GetTimeSeconds(); }
	}
}

TStatId UTownProgressSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UTownProgressSubsystem, STATGROUP_Tickables); }

void UTownProgressSubsystem::GenerateFacilities()
{
	ClearFacilities(); LoadProgress();
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	const auto& World = Settings->MineableLevelGeneration;
	const auto& Town = Settings->TownFacilities;
	if (Town.bEnabled == false || World.WidthInBlocks < 112) { return; }
	ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	auto Place = [&](EMineFacilityKind Kind, int32 Offset, int32 Depth)
	{
		const int32 Column = World.StartColumn + Offset;
		if (Column < World.StartColumn + 4 || Column >= World.StartColumn + World.WidthInBlocks - 4) { return; }
		const FName ArtKey = Kind == EMineFacilityKind::RailFactory ? TEXT("RailFactory") : (Kind == EMineFacilityKind::Elevator ? TEXT("Elevator") : (Kind == EMineFacilityKind::HouseA ? TEXT("HouseA") : TEXT("HouseB")));
		const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
		UPaperSprite* Sprite = Style != nullptr ? Style->TownSprites.FindRef(ArtKey) : nullptr;
		const FVector2D Pixels = Sprite != nullptr ? Sprite->GetSourceSize() : FVector2D(64.f, 96.f);
		const int32 HalfWidth = FMath::CeilToInt(Pixels.X / 64.f);
		const FVector Floor = Level->PrepareFacilitySite(Column, HalfWidth, FMath::CeilToInt(Pixels.Y / 32.f) + 1, Depth);
		AMineFacilityActor* Facility = GetWorld()->SpawnActor<AMineFacilityActor>();
		if (Facility != nullptr) { Facility->InitializeFacility(Kind, Depth, Floor); Facilities.Add(Facility); }
	};
	Place(EMineFacilityKind::HouseA, Town.HouseAColumn, 0);
	Place(EMineFacilityKind::HouseB, Town.HouseBColumn, 0);
	Place(EMineFacilityKind::Elevator, Town.ElevatorColumn, 0);
	Place(EMineFacilityKind::RailFactory, Town.RailFactoryColumn, 0);
	for (int32 Depth : Town.ElevatorDepths) { if (Depth > 0 && Depth < World.DepthInBlocks - 6) { Place(EMineFacilityKind::Elevator, Town.ElevatorColumn, Depth); } }
	GenerateVillageBackdrop();
}

void UTownProgressSubsystem::ClearFacilities()
{
	for (const auto& Actor : Facilities) { if (Actor.IsValid()) { Actor->Destroy(); } }
	Facilities.Reset();
	for (const auto& Scenery : VillageScenery) { if (Scenery.IsValid()) { Scenery->Destroy(); } }
	VillageScenery.Reset();
}

void UTownProgressSubsystem::WriteSaveData(UMKSaveGame* Save) const
{
	if (Save == nullptr) { return; }
	Save->MaximumReachedDepth = MaximumDepth; Save->TownQuests = Quests; Save->OwnedRelics = OwnedRelics;
}

void UTownProgressSubsystem::RestoreProgress(const UMKSaveGame* Save)
{
	if (Save == nullptr) { return; }
	MaximumDepth = Save->MaximumReachedDepth; Quests = Save->TownQuests; Quests.SetNum(2); OwnedRelics = Save->OwnedRelics; bLoaded = true;
	OnTownProgressChanged.Broadcast();
}

void UTownProgressSubsystem::ClearQuestCooldownsForPlaytest()
{
	const USaveGameSubsystem* Saves = USaveGameSubsystem::Get(this);
	if (Saves == nullptr || Saves->IsPlaytestSession() == false) { return; }
	for (auto& Quest : Quests) { Quest.NextOfferAt = 0; Quest.NextConversationRollAt = 0; }
	OnTownProgressChanged.Broadcast();
}

bool UTownProgressSubsystem::IsSurfaceSiteReserved(int32 Column, int32 HalfWidth) const
{
	const auto* Settings = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset();
	if (Settings->TownFacilities.bEnabled == false || Settings->MineableLevelGeneration.WidthInBlocks < 112) { return false; }
	const auto& Town = Settings->TownFacilities;
	const auto* Style = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetVisualStyleDataAsset();
	for (const auto& Site : { TPair<FName, int32>(TEXT("HouseA"), Town.HouseAColumn), TPair<FName, int32>(TEXT("HouseB"), Town.HouseBColumn), TPair<FName, int32>(TEXT("Elevator"), Town.ElevatorColumn), TPair<FName, int32>(TEXT("RailFactory"), Town.RailFactoryColumn), TPair<FName, int32>(TEXT("VillageNotice"), Town.VillageNoticeColumn) })
	{
		UPaperSprite* Sprite = Style != nullptr ? Style->TownSprites.FindRef(Site.Key) : nullptr;
		const int32 Radius = Sprite != nullptr ? FMath::CeilToInt(Sprite->GetSourceSize().X / 64.f) + 1 : 4;
		if (FMath::Abs(Column - (Settings->MineableLevelGeneration.StartColumn + Site.Value)) <= HalfWidth + Radius) { return true; }
	}
	return false;
}

bool UTownProgressSubsystem::IsRailFactoryUnlocked() const
{
	return MaximumDepth >= UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset()->TownFacilities.RailFactoryUnlockDepth;
}

bool UTownProgressSubsystem::CanAccessRailFactory(const AActor* Player) const
{
	if (IsRailFactoryUnlocked() == false) { return false; }
	for (const auto& Facility : Facilities) { if (Facility.IsValid() && Facility->GetFacilityKind() == EMineFacilityKind::RailFactory && Facility->CanInteract(const_cast<AActor*>(Player))) { return true; } }
	return false;
}

bool UTownProgressSubsystem::CanTravelToDepth(int32 Depth) const
{
	if (Depth < 0 || Depth > MaximumDepth) { return false; }
	for (const auto& Facility : Facilities) { if (Facility.IsValid() && Facility->GetFacilityKind() == EMineFacilityKind::Elevator && Facility->GetStationDepth() == Depth) { return true; } }
	return false;
}

bool UTownProgressSubsystem::TryTravel(AMKCharacter* Player, AMineFacilityActor* Source, int32 Depth)
{
	if (Player == nullptr || Source == nullptr || Source->GetFacilityKind() != EMineFacilityKind::Elevator || Source->CanInteract(Player) == false || CanTravelToDepth(Depth) == false || Source->GetStationDepth() == Depth) { return false; }
	for (const auto& Target : Facilities)
	{
		if (Target.IsValid() && Target->GetFacilityKind() == EMineFacilityKind::Elevator && Target->GetStationDepth() == Depth)
		{
			const FVector Exit = Target->GetActorLocation() + FVector(24.f, 0.f, -2.96f);
			Player->GetCharacterMovement()->StopMovementImmediately();
			Player->SetActorLocation(Exit, false, nullptr, ETeleportType::TeleportPhysics);
			Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Player->TrySaveGame(); return true;
		}
	}
	return false;
}

void UTownProgressSubsystem::BeginResidentConversation(AMKCharacter* Player, AMineFacilityActor* Resident)
{
	if (Player == nullptr || Resident == nullptr || Resident->CanInteract(Player) == false || (Resident->GetFacilityKind() != EMineFacilityKind::HouseA && Resident->GetFacilityKind() != EMineFacilityKind::HouseB)) { return; }
	LoadProgress();
	const int32 Index = Resident->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
	FTownQuestState& State = Quests[Index];
	if (State.bActive || State.bOffered) { return; }
	++State.ConversationCount;
	const auto& Settings = UDataManager::Get(this)->GetGameSettingDataAsset()->TownFacilities;
	const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
	if (State.ConversationCount > Settings.MinimumSmallTalkCount && Now >= State.NextOfferAt && Now >= State.NextConversationRollAt)
	{
		State.NextConversationRollAt = Now + FMath::Max(0, Settings.ConversationRollCooldownSeconds);
		FRandomStream Random(GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->GetLastGenerationSeed() ^ ((Index + 1) * 11213 + State.ConversationCount * 73693));
		TArray<int32> Eligible;
		for (int32 QuestIndex = 0; QuestIndex < Settings.Quests.Num(); ++QuestIndex) { if (Settings.Quests[QuestIndex].MinimumDepth <= MaximumDepth) { Eligible.Add(QuestIndex); } }
		if (Eligible.IsEmpty() == false && Random.FRand() < Settings.QuestOfferChance)
		{
			State.bOffered = true; State.OfferedQuestIndex = Eligible[Random.RandRange(0, Eligible.Num() - 1)];
		}
	}
	bProgressDirty = true; OnTownProgressChanged.Broadcast(); Player->TrySaveGame();
}

bool UTownProgressSubsystem::TryQuestAction(AMKCharacter* Player, AMineFacilityActor* Resident)
{
	if (Player == nullptr || Resident == nullptr || Resident->CanInteract(Player) == false || (Resident->GetFacilityKind() != EMineFacilityKind::HouseA && Resident->GetFacilityKind() != EMineFacilityKind::HouseB)) { return false; }
	LoadProgress();
	const int32 Index = Resident->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
	const FTownQuestDefinition* Definition = GetQuestDefinition(Index);
	if (Definition == nullptr || CanUseQuestAction(Resident, Player) == false) { return false; }
	FTownQuestState& Quest = Quests[Index];
	if (Quest.bActive == false) { Quest.bActive = true; }
	else
	{
		UInventoryComponent* Inventory = Player->FindComponentByClass<UInventoryComponent>();
		if (Inventory->TryRemoveItem(Definition->RequiredItem, Definition->RequiredCount) == false) { return false; }
		UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
		ASC->SetNumericAttributeBase(UAttributeSet_Character::GetCoinAttribute(), ASC->GetNumericAttribute(UAttributeSet_Character::GetCoinAttribute()) + Definition->RewardCoins);
		Quest.bActive = false; Quest.bOffered = false; Quest.OfferedQuestIndex = INDEX_NONE; ++Quest.CompletedCount;
		const auto& Town = UDataManager::Get(this)->GetGameSettingDataAsset()->TownFacilities;
		Quest.NextOfferAt = FDateTime::UtcNow().ToUnixTimestamp() + FMath::Max(0, Town.QuestCooldownSeconds);
		FRandomStream Random(GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->GetLastGenerationSeed() ^ ((Index + 1) * 7919 + Quest.CompletedCount * 104729));
		if (Random.FRand() < Town.QuestRelicChance || Quest.CompletedCount >= FMath::Max(1, Town.GuaranteedRelicQuestCount)) { OwnedRelics.Add(TEXT("SteadyBelt")); }
	}
	bProgressDirty = true; OnTownProgressChanged.Broadcast(); Player->TrySaveGame(); return true;
}

int32 UTownProgressSubsystem::GetMaximumDepth() const { return MaximumDepth; }
bool UTownProgressSubsystem::HasRelic(FName Key) const { return OwnedRelics.Contains(Key); }

FText UTownProgressSubsystem::GetDialogue(const AMineFacilityActor* Facility, AMKCharacter* Player) const
{
	if (Facility == nullptr) { return FText::GetEmpty(); }
	const auto* Settings = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset();
	if (Facility->GetFacilityKind() == EMineFacilityKind::RailFactory)
	{
		return IsRailFactoryUnlocked() ? NSLOCTEXT("Town", "FactoryOpen", "철도와 광산 열차를 준비해 두었습니다.") : FText::Format(NSLOCTEXT("Town", "FactoryConstruction", "아직 공사 중입니다.\n{0}m까지 탐사하면 철도 공장이 열립니다."), FText::AsNumber(Settings->TownFacilities.RailFactoryUnlockDepth));
	}
	if (Facility->GetFacilityKind() == EMineFacilityKind::Elevator)
	{
		const bool bAnyStation = Settings->TownFacilities.ElevatorDepths.ContainsByPredicate([&](int32 Depth) { return CanTravelToDepth(Depth); });
		return bAnyStation ? FText::Format(NSLOCTEXT("Town", "ChooseStation", "이동할 정거장을 선택하세요.\n최대 도달 깊이 {0}m"), FText::AsNumber(MaximumDepth)) : NSLOCTEXT("Town", "ElevatorConstruction", "아직 공사 중입니다.\n지하 정거장의 깊이까지 탐사하면 연결됩니다.");
	}
	const int32 Index = Facility->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
	const FTownQuestDefinition* Definition = GetQuestDefinition(Index);
	if (Definition == nullptr)
	{
		const FText Miner[] = {NSLOCTEXT("Town", "MinerChat1", "갱도에 들어가기 전엔 산소를 확인해 두세요."), NSLOCTEXT("Town", "MinerChat2", "오늘은 지상이 조용하네요. 잠시 쉬었다 가세요."), NSLOCTEXT("Town", "MinerChat3", "좋은 돌도 서두르다 놓칠 때가 있죠.")};
		const FText Mechanic[] = {NSLOCTEXT("Town", "MechanicChat1", "수레는 출발보다 멈추는 쪽이 더 중요하죠."), NSLOCTEXT("Town", "MechanicChat2", "경사에선 수레가 굴러갈 수 있으니 조심하세요."), NSLOCTEXT("Town", "MechanicChat3", "낡은 바퀴도 손보면 다시 쓸 수 있어요.")};
		const int32 Line = FMath::Max(0, Quests[Index].ConversationCount - 1) % 3;
		return Index == 0 ? Miner[Line] : Mechanic[Line];
	}
	if (Definition->MinimumDepth > MaximumDepth) { return FText::Format(NSLOCTEXT("Town", "QuestDepth", "{0}m 부근을 탐사한 뒤 다시 찾아와 주세요."), FText::AsNumber(Definition->MinimumDepth)); }
	const int32 Owned = Player != nullptr ? Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(Definition->RequiredItem) : 0;
	return FText::Format(NSLOCTEXT("Town", "ResidentQuestDescription", "{0}\n\n{1}  {2} / {3}\n보상  {4} 재화"), Definition->Title, UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetItemDisplayName(Definition->RequiredItem), FText::AsNumber(Owned), FText::AsNumber(Definition->RequiredCount), FText::AsNumber(Definition->RewardCoins));
}

FText UTownProgressSubsystem::GetQuestActionLabel(const AMineFacilityActor* Facility) const
{
	const int32 Index = Facility->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
	if (GetQuestDefinition(Index) == nullptr) { return FText::GetEmpty(); }
	return Quests.IsValidIndex(Index) && Quests[Index].bActive ? NSLOCTEXT("Town", "TurnIn", "납품하고 보상 받기") : NSLOCTEXT("Town", "Accept", "의뢰 수락");
}

bool UTownProgressSubsystem::CanUseQuestAction(const AMineFacilityActor* Facility, AMKCharacter* Player) const
{
	if (Facility == nullptr || Player == nullptr) { return false; }
	const int32 Index = Facility->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
	const FTownQuestDefinition* Definition = GetQuestDefinition(Index);
	if (Definition == nullptr || Definition->MinimumDepth > MaximumDepth) { return false; }
	if (Quests[Index].bActive) { return Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(Definition->RequiredItem) >= Definition->RequiredCount; }
	return Quests[Index].NextOfferAt <= FDateTime::UtcNow().ToUnixTimestamp();
}

const TArray<TWeakObjectPtr<AMineFacilityActor>>& UTownProgressSubsystem::GetFacilities() const { return Facilities; }

void UTownProgressSubsystem::LoadProgress()
{
	if (bLoaded) { return; }
	USaveGameSubsystem* Saves = USaveGameSubsystem::Get(this);
	const UMKSaveGame* Save = Saves != nullptr ? Saves->GetSaveGame() : nullptr;
	if (Save != nullptr) { MaximumDepth = Save->MaximumReachedDepth; Quests = Save->TownQuests; OwnedRelics = Save->OwnedRelics; }
	Quests.SetNum(2); bLoaded = true;
}

void UTownProgressSubsystem::GenerateVillageBackdrop()
{
	const auto* Settings = UDataManager::Get(this)->GetGameSettingDataAsset();
	const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
	if (Style == nullptr) { return; }
	auto Place = [&](FName Key, int32 Offset)
	{
		UPaperSprite* Sprite = Style->TownSprites.FindRef(Key);
		const int32 Column = Settings->MineableLevelGeneration.StartColumn + Offset;
		if (Sprite == nullptr || Column < Settings->MineableLevelGeneration.StartColumn || Offset >= Settings->MineableLevelGeneration.WidthInBlocks) { return; }
		const float Floor = -GetWorld()->GetSubsystem<ULevelManagerSubsystem>()->GetSurfaceRow(Column) * 16.f;
		const FVector Location((Column + 0.5f) * 16.f, Key == TEXT("VillageBackdrop") ? -80.f : -42.f, Floor + Sprite->GetSourceSize().Y / Sprite->GetPixelsPerUnrealUnit() * 0.5f);
		APaperSpriteActor* Actor = GetWorld()->SpawnActor<APaperSpriteActor>(APaperSpriteActor::StaticClass(), Location, FRotator::ZeroRotator);
		if (Actor == nullptr) { return; }
		Actor->SetActorEnableCollision(false); Actor->Tags.Add(TEXT("VillageScenery"));
		Actor->SetActorTickEnabled(false);
		auto* Render = Actor->GetRenderComponent(); Render->SetMobility(EComponentMobility::Movable); Render->SetSprite(Sprite); Render->SetCollisionEnabled(ECollisionEnabled::NoCollision); Render->SetTranslucentSortPriority(-8); Render->SetComponentTickEnabled(false);
		VillageScenery.Add(Actor);
	};
	for (int32 Column : Settings->TownFacilities.VillageBackdropColumns) { Place(TEXT("VillageBackdrop"), Column); }
	Place(TEXT("VillageNotice"), Settings->TownFacilities.VillageNoticeColumn);
}

const FTownQuestDefinition* UTownProgressSubsystem::GetQuestDefinition(int32 Resident) const
{
	if (Quests.IsValidIndex(Resident) == false) { return nullptr; }
	const auto& List = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset()->TownFacilities.Quests;
	const auto& State = Quests[Resident];
	if (List.IsEmpty() || (State.bActive == false && State.bOffered == false)) { return nullptr; }
	const int32 QuestIndex = List.IsValidIndex(State.OfferedQuestIndex) ? State.OfferedQuestIndex : (State.CompletedCount + Resident) % List.Num();
	return &List[QuestIndex];
}
