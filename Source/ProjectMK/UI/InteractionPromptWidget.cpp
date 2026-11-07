// LINK

#include "ProjectMK/UI/InteractionPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UInteractionPromptWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UInteractionPromptWidget::BuildDefaultWidgetTree()
{
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InteractionPrompt"));
	RootSize->SetWidthOverride(36.f);
	RootSize->SetHeightOverride(36.f);
	WidgetTree->RootWidget = RootSize;

	UBorder* Keycap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Keycap"));
	Keycap->SetPadding(FMargin(FMKUILayout::SpaceXS));
	RootSize->SetContent(Keycap);

	UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyText"));
	KeyText->SetText(FText::FromString(TEXT("F")));
	KeyText->SetJustification(ETextJustify::Center);
	ApplyTextRole(KeyText, EMKTextRole::Button);
	Keycap->SetContent(KeyText);
}
