#include "ProjectMK/UI/BountyNoticeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "PaperSprite.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"

TSharedRef<SWidget> UBountyNoticeWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(); WidgetTree->RootWidget = Root;
		UImage* Paper = WidgetTree->ConstructWidget<UImage>();
		const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
		FSlateBrush Brush; Brush.SetResourceObject(Style != nullptr ? Style->TownSprites.FindRef(TEXT("BountyPaper")) : nullptr);
		Brush.ImageSize = FVector2D(320.f,192.f); Brush.DrawAs = ESlateBrushDrawType::Box; Brush.Margin = FMargin(32.f/320.f,32.f/192.f);
		Paper->SetBrush(Brush); Paper->SetVisibility(ESlateVisibility::HitTestInvisible); auto* PaperLayout = Root->AddChildToOverlay(Paper); PaperLayout->SetHorizontalAlignment(HAlign_Fill); PaperLayout->SetVerticalAlignment(VAlign_Fill);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(); auto* ContentLayout = Root->AddChildToOverlay(Column); ContentLayout->SetPadding(FMargin(24.f,28.f,24.f,20.f)); ContentLayout->SetHorizontalAlignment(HAlign_Fill); ContentLayout->SetVerticalAlignment(VAlign_Fill);
		UHorizontalBox* Heading = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
		USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(); PortraitSize->SetWidthOverride(54.f); PortraitSize->SetHeightOverride(54.f);
		Portrait = WidgetTree->ConstructWidget<UImage>(); PortraitSize->SetContent(Portrait); Heading->AddChildToHorizontalBox(PortraitSize)->SetPadding(FMargin(0.f,0.f,18.f,0.f));
		Title = WidgetTree->ConstructWidget<UTextBlock>(); ApplyTextRole(Title,EMKTextRole::SectionTitle,true); Title->SetColorAndOpacity(FLinearColor(0.18f,0.09f,0.035f));
		Heading->AddChildToHorizontalBox(Title)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Objective = WidgetTree->ConstructWidget<UTextBlock>(); ApplyTextRole(Objective,EMKTextRole::Body,true); Objective->SetColorAndOpacity(FLinearColor(0.22f,0.12f,0.06f));
		Column->AddChildToVerticalBox(Objective)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0.f,12.f,0.f,0.f));
		UTextBlock* RewardText = nullptr;
		Footer->AddChildToHorizontalBox(BuildCurrencyAmount(RewardText,FText::GetEmpty(),EMKTextRole::Numeric,22.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Reward = RewardText; Reward->SetColorAndOpacity(FLinearColor(0.25f,0.12f,0.04f));
		UButton* Button = nullptr; UTextBlock* ButtonText = nullptr;
		Footer->AddChildToHorizontalBox(BuildActionButton(Button,ButtonText,FText::GetEmpty(),128.f,38.f));
		Action = Button; ActionLabel = ButtonText;
		Action->OnClicked.AddUniqueDynamic(this,&UBountyNoticeWidget::OnAction);
		UButton* Abandon = nullptr; UTextBlock* AbandonLabel = nullptr;
		AbandonFrame = BuildActionButton(Abandon,AbandonLabel,NSLOCTEXT("Board","Abandon","포기"),72.f,38.f);
		Footer->AddChildToHorizontalBox(AbandonFrame)->SetPadding(FMargin(8.f,0.f,0.f,0.f));
		Abandon->OnClicked.AddUniqueDynamic(this,&UBountyNoticeWidget::OnAbandon);
	}
	return Super::RebuildWidget();
}

void UBountyNoticeWidget::SetContract(AMineFacilityActor* Board, int32 Index)
{
	if (Title == nullptr) { return; }
	const auto* Town = GetWorld()->GetSubsystem<UTownProgressSubsystem>();
	const auto* Quest = Town->GetBoardQuestDefinition(Index); const auto* State = Town->GetBoardQuestState(Index);
	SetVisibility(Quest != nullptr ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (Quest == nullptr) { return; }
	const auto* Monster = UDataManager::Get(this)->GetMonsterDataTableRow(Quest->TargetMonster);
	Portrait->SetBrushResourceObject(Monster != nullptr ? Monster->Sprite.LoadSynchronous() : nullptr); Portrait->SetVisibility(Monster != nullptr && Monster->Sprite.IsNull() == false ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	Title->SetText(Quest->Title);
	Objective->SetText(FText::Format(NSLOCTEXT("Board","Objective","{0} 퇴치\n{1} / {2}"),Monster != nullptr ? Monster->DisplayName : FText::FromName(Quest->TargetMonster),FText::AsNumber(State != nullptr ? State->MonsterKillProgress : 0),FText::AsNumber(Quest->RequiredCount)));
	Reward->SetText(FText::AsNumber(Quest->RewardCoins));
	FText Label = Town->GetQuestActionLabel(Board,Index);
	ActionLabel->SetText(Label); Action->SetVisibility(ESlateVisibility::Visible); Action->SetIsEnabled(Town->CanUseQuestAction(Board,GetLocalPlayerCharacter(),Index));
	AbandonFrame->SetVisibility(State != nullptr && State->bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UBountyNoticeWidget::OnAction() { OnChosen.ExecuteIfBound(); }

void UBountyNoticeWidget::OnAbandon() { OnAbandoned.ExecuteIfBound(); }
