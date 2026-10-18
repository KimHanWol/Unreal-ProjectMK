#include "ProjectMK/UI/PlaytestWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Monster/MKMonster.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Core/Subsystem/SaveGameSubsystem.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/UI/MKMockupButton.h"

TSharedRef<SWidget> UPlaytestWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}
	return Super::RebuildWidget();
}

void UPlaytestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	GetWorld()->GetTimerManager().SetTimer(StatusTimerHandle, this, &UPlaytestWidget::UpdateStatus, 0.25f, true);
}

void UPlaytestWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StatusTimerHandle);
	}
	Super::NativeDestruct();
}

void UPlaytestWidget::BindEvents()
{
	TogglePanelButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnTogglePanelClicked);
	MaterialsButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnMaterialsClicked);
	ConsumablesButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnConsumablesClicked);
	CoinsButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnCoinsClicked);
	ShopButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnShopClicked);
	CraftingButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnCraftingClicked);
	SkillsButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnSkillsClicked);
	DamageButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnDamageClicked);
	DrainOxygenButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnDrainOxygenClicked);
	RestoreButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnRestoreClicked);
	ReturnButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnReturnClicked);
	SpawnMonsterButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnSpawnMonsterClicked);
	HitMonsterButton->OnClicked.AddUniqueDynamic(this, &UPlaytestWidget::OnHitMonsterClicked);
}

void UPlaytestWidget::UnbindEvents()
{
	TogglePanelButton->OnClicked.RemoveAll(this);
	MaterialsButton->OnClicked.RemoveAll(this);
	ConsumablesButton->OnClicked.RemoveAll(this);
	CoinsButton->OnClicked.RemoveAll(this);
	ShopButton->OnClicked.RemoveAll(this);
	CraftingButton->OnClicked.RemoveAll(this);
	SkillsButton->OnClicked.RemoveAll(this);
	DamageButton->OnClicked.RemoveAll(this);
	DrainOxygenButton->OnClicked.RemoveAll(this);
	RestoreButton->OnClicked.RemoveAll(this);
	ReturnButton->OnClicked.RemoveAll(this);
	SpawnMonsterButton->OnClicked.RemoveAll(this);
	HitMonsterButton->OnClicked.RemoveAll(this);
}

void UPlaytestWidget::UpdateWidget()
{
	UpdateStatus();
}

void UPlaytestWidget::TogglePanel()
{
	DetailsPanel->SetVisibility(DetailsPanel->GetVisibility() == ESlateVisibility::Collapsed
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UPlaytestWidget::BuildDefaultWidgetTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(320.f);
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
	PanelSlot->SetPosition(FVector2D(12.f, 12.f));
	PanelSlot->SetAutoSize(true);
	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>();
	PanelBorder->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.94f));
	PanelBorder->SetPadding(FMargin(12.f));
	PanelSize->SetContent(PanelBorder);
	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelBorder->SetContent(Panel);
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	Panel->AddChildToVerticalBox(TitleRow);
	TogglePanelButton = AddActionButton(TitleRow, NSLOCTEXT("Playtest", "Title", "PROJECT MK · 목업 테스트 / F1"), NSLOCTEXT("Playtest", "FoldHint", "테스트 패널 펼치기 / 접기"));
	StatusText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 12;
	StatusText->SetFont(StatusFont);
	Panel->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 8.f));
	DetailsPanel = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChildToVerticalBox(DetailsPanel);

	UTextBlock* Controls = WidgetTree->ConstructWidget<UTextBlock>();
	Controls->SetText(NSLOCTEXT("Playtest", "Controls", "A/D 이동 · WASD 방향 채굴\nSpace 비행 · R 누르기 자석\nE / 클릭 상점 · C 제작 · K 스킬\n1~8 / 하단 클릭 아이템 사용"));
	Controls->SetFont(StatusFont);
	Controls->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.8f, 0.9f)));
	DetailsPanel->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	UHorizontalBox* MenuRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DetailsPanel->AddChildToVerticalBox(MenuRow);
	ShopButton = AddActionButton(MenuRow, NSLOCTEXT("Playtest", "Shop", "상점"), NSLOCTEXT("Playtest", "ShopHint", "거리 조건 없이 상점 열기"));
	CraftingButton = AddActionButton(MenuRow, NSLOCTEXT("Playtest", "Crafting", "제작 [C]"), FText::GetEmpty());
	SkillsButton = AddActionButton(MenuRow, NSLOCTEXT("Playtest", "Skills", "스킬 [K]"), FText::GetEmpty());
	UHorizontalBox* KitRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DetailsPanel->AddChildToVerticalBox(KitRow);
	MaterialsButton = AddActionButton(KitRow, NSLOCTEXT("Playtest", "Materials", "재료 세트"), NSLOCTEXT("Playtest", "MaterialsHint", "이번 플레이 저장 중단 · 인벤토리를 제작 재료로 교체 (Slime 포함)"));
	ConsumablesButton = AddActionButton(KitRow, NSLOCTEXT("Playtest", "Consumables", "소모품 세트"), NSLOCTEXT("Playtest", "ConsumablesHint", "이번 플레이 저장 중단 · 인벤토리를 소모품으로 교체"));
	UHorizontalBox* CoinRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DetailsPanel->AddChildToVerticalBox(CoinRow);
	CoinsButton = AddActionButton(CoinRow, NSLOCTEXT("Playtest", "Coins", "+1,000 코인"), NSLOCTEXT("Playtest", "CoinsHint", "이번 플레이 저장 중단 · 구매 테스트용 코인 지급"));
	ReturnButton = AddActionButton(CoinRow, NSLOCTEXT("Playtest", "Return", "지상 복귀"), NSLOCTEXT("Playtest", "ReturnHint", "이번 플레이 저장 중단 · 스폰 위치로 이동"));
	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DetailsPanel->AddChildToVerticalBox(StatusRow);
	DamageButton = AddActionButton(StatusRow, NSLOCTEXT("Playtest", "Damage", "체력 -25"), FText::GetEmpty());
	DrainOxygenButton = AddActionButton(StatusRow, NSLOCTEXT("Playtest", "DrainOxygen", "산소 -40%"), NSLOCTEXT("Playtest", "OxygenHint", "지하에서 사용하세요. 지상에서는 산소가 즉시 회복됩니다."));
	RestoreButton = AddActionButton(StatusRow, NSLOCTEXT("Playtest", "Restore", "회복"), FText::GetEmpty());
	UHorizontalBox* MonsterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	DetailsPanel->AddChildToVerticalBox(MonsterRow);
	SpawnMonsterButton = AddActionButton(MonsterRow, NSLOCTEXT("Playtest", "SpawnMonster", "몬스터 소환"), NSLOCTEXT("Playtest", "SpawnMonsterHint", "이번 플레이 저장 중단 · 주변에 DataTable 몬스터 1마리 소환"));
	HitMonsterButton = AddActionButton(MonsterRow, NSLOCTEXT("Playtest", "HitMonster", "몬스터 자극"), NSLOCTEXT("Playtest", "HitMonsterHint", "소환한 몬스터에게 10 피해 · 분노 드릴링 확인"));
	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>();
	FeedbackText->SetFont(StatusFont);
	FeedbackText->SetAutoWrapText(true);
	FeedbackText->SetText(NSLOCTEXT("Playtest", "TestGuide", "추천: 재료 세트 → 코인 지급 → 상점에서 레시피 구매 → C 제작 → 지하에서 사용\n세트 버튼은 이번 플레이의 인벤토리를 교체합니다. 테스트 조작 후에는 종료까지 파일에 저장하지 않습니다."));
	DetailsPanel->AddChildToVerticalBox(FeedbackText)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

UButton* UPlaytestWidget::AddActionButton(UHorizontalBox* Row, const FText& Label, const FText& Hint)
{
	UButton* Button = WidgetTree->ConstructWidget<UMKMockupButton>();
	Button->SetBackgroundColor(FLinearColor(0.1f, 0.25f, 0.33f));
	Button->SetToolTipText(Hint);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(Label);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 12;
	Text->SetFont(Font);
	Text->SetJustification(ETextJustify::Center);
	Button->SetContent(Text);
	UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
	ButtonSlot->SetPadding(FMargin(2.f, 3.f));
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Button;
}

void UPlaytestWidget::UpdateStatus()
{
	const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
	const USaveGameSubsystem* Saves = USaveGameSubsystem::Get(this);
	const ULevelManagerSubsystem* Level = GetWorld()->GetSubsystem<ULevelManagerSubsystem>();
	if (::IsValid(Attributes))
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f/%.0f   O2 %.0f/%.0f   코인 %.0f\n시드 %d · %s"),
			Attributes->GetCurrentHealth(), Attributes->GetMaxHealth(), Attributes->GetCurrentOxygen(), Attributes->GetMaxOxygen(), Attributes->GetCoin(),
			::IsValid(Level) ? Level->GetLastGenerationSeed() : INDEX_NONE,
			::IsValid(Saves) && Saves->IsPlaytestSession() ? TEXT("테스트 · 파일 저장 꺼짐") : TEXT("일반 플레이 · 자동 저장"))));
	}
}

bool UPlaytestWidget::TryStartPlaytestSession()
{
	USaveGameSubsystem* Saves = USaveGameSubsystem::Get(this);
	return ::IsValid(GetLocalPlayerCharacter()) && ::IsValid(Saves) && Saves->StartPlaytestSession();
}

void UPlaytestWidget::ApplyTestInventory(bool bConsumables)
{
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	const UDataManager* Data = UDataManager::Get(this);
	if (::IsValid(Inventory) == false || ::IsValid(Data) == false || TryStartPlaytestSession() == false)
	{
		return;
	}
	TMap<FName, int32> TestItems;
	for (const FName& RecipeKey : Data->GetShopRecipeRowNames())
	{
		const FShopRecipeDataTableRow* Recipe = Data->GetShopRecipeDataTableRow(RecipeKey);
		const FItemDataTableRow* Result = Recipe != nullptr ? Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Recipe->GetResultItemKey()) : nullptr;
		if (Result == nullptr || Result->ConsumableEffect == EConsumableEffectType::None)
		{
			continue;
		}
		if (bConsumables)
		{
			TestItems.FindOrAdd(Recipe->GetResultItemKey()) = FMath::Min(3, FMath::Max(1, Result->MaxStackCount));
		}
		else
		{
			for (const FShopRecipeItem& Required : Recipe->RequiredItems)
			{
				const FItemDataTableRow* Material = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Required.GetItemKey());
				if (Material != nullptr && Material->bIsInventoryItem)
				{
					TestItems.FindOrAdd(Required.GetItemKey()) = FMath::Min(30, FMath::Max(1, Material->MaxStackCount));
				}
			}
		}
	}
	if (TestItems.IsEmpty())
	{
		FeedbackText->SetText(NSLOCTEXT("Playtest", "MissingData", "소모품 레시피가 없습니다. 데이터테이블 연결을 확인하세요."));
		return;
	}
	TArray<FName> Keys;
	TestItems.GenerateKeyArray(Keys);
	Keys.Sort(FNameLexicalLess());
	GetLocalPlayerCharacter()->GetConsumableComponent()->ClearRecall();
	Inventory->ApplySavedInventorySlots(TArray<FInventorySlotData>());
	int32 Granted = 0;
	for (const FName& Key : Keys)
	{
		Granted += Inventory->AddItem(Key, TestItems[Key]) ? 1 : 0;
	}
	FeedbackText->SetText(FText::Format(NSLOCTEXT("Playtest", "KitGranted", "인벤토리 교체: {0}/{1}종 지급. 이번 플레이는 저장하지 않습니다."), Granted, Keys.Num()));
	UpdateWidget();
}

void UPlaytestWidget::OnTogglePanelClicked()
{
	TogglePanel();
}

void UPlaytestWidget::OnMaterialsClicked()
{
	ApplyTestInventory(false);
}

void UPlaytestWidget::OnConsumablesClicked()
{
	ApplyTestInventory(true);
}

void UPlaytestWidget::OnCoinsClicked()
{
	if (TryStartPlaytestSession())
	{
		USkillComponent* Skills = GetLocalSkillComponent();
		if (::IsValid(Skills) && Skills->TryAddCoin(1000, false))
		{
			FeedbackText->SetText(NSLOCTEXT("Playtest", "CoinsGranted", "1,000 코인 지급 · 상점 레시피와 스킬을 구매할 수 있습니다."));
		}
		UpdateStatus();
	}
}

void UPlaytestWidget::OnShopClicked()
{
	AMKPlayerController* Controller = Cast<AMKPlayerController>(GetOwningPlayer());
	if (::IsValid(Controller))
	{
		Controller->TryOpenShop();
	}
}

void UPlaytestWidget::OnCraftingClicked()
{
	AMKPlayerController* Controller = Cast<AMKPlayerController>(GetOwningPlayer());
	if (::IsValid(Controller))
	{
		Controller->ToggleCrafting();
	}
}

void UPlaytestWidget::OnSkillsClicked()
{
	AMKPlayerController* Controller = Cast<AMKPlayerController>(GetOwningPlayer());
	if (::IsValid(Controller))
	{
		Controller->ToggleSkillTree();
	}
}

void UPlaytestWidget::OnDamageClicked()
{
	if (TryStartPlaytestSession())
	{
		FGameplayAbilityUtils::ApplyDamage(GetOwnerAbilitySystemComponent(), nullptr, 25.f);
		UpdateStatus();
	}
}

void UPlaytestWidget::OnDrainOxygenClicked()
{
	const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
	if (::IsValid(Attributes) && TryStartPlaytestSession())
	{
		FGameplayAbilityUtils::ApplyOxygen(GetOwnerAbilitySystemComponent(), -Attributes->GetMaxOxygen() * 0.4f);
		UpdateStatus();
	}
}

void UPlaytestWidget::OnRestoreClicked()
{
	const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
	if (::IsValid(Attributes) && TryStartPlaytestSession())
	{
		FGameplayAbilityUtils::ApplyDamage(GetOwnerAbilitySystemComponent(), nullptr, -Attributes->GetMaxHealth());
		FGameplayAbilityUtils::ApplyOxygen(GetOwnerAbilitySystemComponent(), Attributes->GetMaxOxygen());
		UpdateStatus();
	}
}

void UPlaytestWidget::OnReturnClicked()
{
	if (TryStartPlaytestSession())
	{
		GetLocalPlayerCharacter()->GetConsumableComponent()->ClearRecall();
		GetLocalPlayerCharacter()->RespawnAtSpawnLocation();
		UpdateStatus();
	}
}

void UPlaytestWidget::OnSpawnMonsterClicked()
{
	const UDataManager* Data = UDataManager::Get(this);
	if (::IsValid(Data) == false || Data->GetMonsterDataTableRow(MonsterRowKey) == nullptr)
	{
		FeedbackText->SetText(NSLOCTEXT("Playtest", "MissingMonster", "Monster DataTable과 몬스터 행 연결을 확인하세요."));
		return;
	}
	if (TryStartPlaytestSession() == false)
	{
		return;
	}
	if (TestMonster.IsValid())
	{
		TestMonster->Destroy();
	}
	AMKCharacter* Character = GetLocalPlayerCharacter();
	const FVector Location = Character->GetActorLocation();
	const float Direction = Character->GetCharacterDirection().X < 0.f ? -1.f : 1.f;
	FTransform SpawnTransform(Location + FVector(Direction * 40.f, 0.f, 0.f));
	AMKMonster* Monster = GetWorld()->SpawnActorDeferred<AMKMonster>(AMKMonster::StaticClass(), SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (::IsValid(Monster))
	{
		Monster->SetMonsterDataRowKey(MonsterRowKey);
		Monster->FinishSpawning(SpawnTransform);
	}
	TestMonster = Monster;
	FeedbackText->SetText(::IsValid(Monster)
		? NSLOCTEXT("Playtest", "MonsterSpawned", "몬스터 소환. 대기·배회 / 접촉 피해 / 드릴 공격 또는 자극 버튼으로 분노를 확인하세요.")
		: NSLOCTEXT("Playtest", "MonsterBlocked", "소환할 공간이 부족합니다. 넓은 지상에서 다시 시도하세요."));
}

void UPlaytestWidget::OnHitMonsterClicked()
{
	if (TestMonster.IsValid() == false)
	{
		FeedbackText->SetText(NSLOCTEXT("Playtest", "SpawnMonsterFirst", "먼저 몬스터를 소환하세요."));
		return;
	}
	if (TryStartPlaytestSession())
	{
		FGameplayAbilityUtils::ApplyDamage(TestMonster->GetAbilitySystemComponent(), GetOwnerAbilitySystemComponent(), 10.f);
		FeedbackText->SetText(NSLOCTEXT("Playtest", "MonsterHit", "몬스터에게 10 피해를 줬습니다. 분노 상태에서는 주변을 무작위로 드릴링합니다."));
	}
}
