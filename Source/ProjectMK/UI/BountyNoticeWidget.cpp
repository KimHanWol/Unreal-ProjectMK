#include "ProjectMK/UI/BountyNoticeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
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
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget = Column;
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
	}
	return Super::RebuildWidget();
}

void UBountyNoticeWidget::SetContract(AMineFacilityActor* Board, int32 Index)
{
	if (Title == nullptr) { return; }
	const auto* Town = GetWorld()->GetSubsystem<UTownProgressSubsystem>();
	const auto* Quest = Town->GetBoardQuestDefinition(Index); const auto* State = Town->GetBoardQuestState(Index);
	if (Quest == nullptr)
	{
		Title->SetText(NSLOCTEXT("Board","Pending","의뢰 준비 중")); Objective->SetText(FText::GetEmpty()); Portrait->SetVisibility(ESlateVisibility::Hidden); Action->SetVisibility(ESlateVisibility::Collapsed); Reward->SetText(FText::GetEmpty()); return;
	}
	const auto* Monster = UDataManager::Get(this)->GetMonsterDataTableRow(Quest->TargetMonster);
	Portrait->SetBrushResourceObject(Monster != nullptr ? Monster->Sprite.LoadSynchronous() : nullptr); Portrait->SetVisibility(Monster != nullptr && Monster->Sprite.IsNull() == false ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	Title->SetText(Quest->Title);
	Objective->SetText(FText::Format(NSLOCTEXT("Board","Objective","{0} 퇴치\n{1} / {2}"),Monster != nullptr ? Monster->DisplayName : FText::FromName(Quest->TargetMonster),FText::AsNumber(State != nullptr ? State->MonsterKillProgress : 0),FText::AsNumber(Quest->RequiredCount)));
	Reward->SetText(FText::AsNumber(Quest->RewardCoins));
	FText Label = Town->GetQuestActionLabel(Board,Index);
	if (Quest->MinimumDepth > Town->GetMaximumDepth()) { Label = FText::Format(NSLOCTEXT("Board","ReachDepth","{0}m 탐사 필요"),FText::AsNumber(Quest->MinimumDepth)); }
	else if (Label.IsEmpty()) { Label = NSLOCTEXT("Board","Preparing","준비 중"); }
	ActionLabel->SetText(Label); Action->SetVisibility(ESlateVisibility::Visible); Action->SetIsEnabled(Town->CanUseQuestAction(Board,GetLocalPlayerCharacter(),Index));
}

void UBountyNoticeWidget::OnAction() { OnChosen.ExecuteIfBound(); }
