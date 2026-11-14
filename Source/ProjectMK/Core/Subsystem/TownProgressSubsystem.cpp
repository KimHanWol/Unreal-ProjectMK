#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
		const int32 HalfWidth = Kind == EMineFacilityKind::RailFactory ? 2 : (Kind == EMineFacilityKind::Elevator ? 1 : 2);
		const FVector Floor = Level->PrepareFacilitySite(Column, HalfWidth, 6, Depth);
		AMineFacilityActor* Facility = GetWorld()->SpawnActor<AMineFacilityActor>();
		if (Facility != nullptr) { Facility->InitializeFacility(Kind, Depth, Floor); Facilities.Add(Facility); }
	};
	Place(EMineFacilityKind::HouseA, Town.HouseAColumn, 0);
	Place(EMineFacilityKind::HouseB, Town.HouseBColumn, 0);
	Place(EMineFacilityKind::Elevator, Town.ElevatorColumn, 0);
	Place(EMineFacilityKind::RailFactory, Town.RailFactoryColumn, 0);
	for (int32 Depth : Town.ElevatorDepths) { if (Depth > 0 && Depth < World.DepthInBlocks - 6) { Place(EMineFacilityKind::Elevator, Town.ElevatorColumn, Depth); } }
}

void UTownProgressSubsystem::ClearFacilities()
{
	for (const auto& Actor : Facilities) { if (Actor.IsValid()) { Actor->Destroy(); } }
	Facilities.Reset();
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
	for (auto& Quest : Quests) { Quest.NextOfferAt = 0; }
	OnTownProgressChanged.Broadcast();
}

bool UTownProgressSubsystem::IsSurfaceSiteReserved(int32 Column, int32 HalfWidth) const
{
	const auto* Settings = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset();
	if (Settings->TownFacilities.bEnabled == false || Settings->MineableLevelGeneration.WidthInBlocks < 112) { return false; }
	const auto& Town = Settings->TownFacilities;
	for (int32 Offset : { Town.HouseAColumn, Town.HouseBColumn, Town.ElevatorColumn, Town.RailFactoryColumn })
	{
		if (FMath::Abs(Column - (Settings->MineableLevelGeneration.StartColumn + Offset)) <= HalfWidth + 4) { return true; }
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
		Quest.bActive = false; ++Quest.CompletedCount;
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
	if (Definition == nullptr) { return NSLOCTEXT("Town", "NoQuest", "지금은 부탁할 일이 없네요."); }
	if (Quests[Index].bActive == false && Quests[Index].NextOfferAt > FDateTime::UtcNow().ToUnixTimestamp()) { return NSLOCTEXT("Town", "QuestCooldown", "도와줘서 고마워요. 다음에 또 들러 주세요."); }
	if (Definition->MinimumDepth > MaximumDepth) { return FText::Format(NSLOCTEXT("Town", "QuestDepth", "{0}m 부근을 탐사한 뒤 다시 찾아와 주세요."), FText::AsNumber(Definition->MinimumDepth)); }
	const int32 Owned = Player != nullptr ? Player->FindComponentByClass<UInventoryComponent>()->GetItemCount(Definition->RequiredItem) : 0;
	return FText::Format(NSLOCTEXT("Town", "QuestDescription", "{0}\n\n{1}  {2} / {3}\n보상  {4} 재화\n\n드물게 유물을 발견할 수도 있어요. 의뢰 3회 완료 시 광부의 중심추를 확실히 드립니다."), Definition->Title, UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetItemDisplayName(Definition->RequiredItem), FText::AsNumber(Owned), FText::AsNumber(Definition->RequiredCount), FText::AsNumber(Definition->RewardCoins));
}

FText UTownProgressSubsystem::GetQuestActionLabel(const AMineFacilityActor* Facility) const
{
	const int32 Index = Facility->GetFacilityKind() == EMineFacilityKind::HouseA ? 0 : 1;
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

const FTownQuestDefinition* UTownProgressSubsystem::GetQuestDefinition(int32 Resident) const
{
	if (Quests.IsValidIndex(Resident) == false) { return nullptr; }
	const auto& List = UDataManager::Get(const_cast<UTownProgressSubsystem*>(this))->GetGameSettingDataAsset()->TownFacilities.Quests;
	return List.IsEmpty() ? nullptr : &List[(Quests[Resident].CompletedCount + Resident) % List.Num()];
}
