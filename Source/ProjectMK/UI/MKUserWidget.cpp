// LINK

#include "ProjectMK/UI/MKUserWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/SpinBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/Paths.h"
#include "Fonts/CompositeFont.h"
#include "Components/TextBlock.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/UI/MKMockupButton.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/UI/HUDWidget.h"

bool UMKUserWidget::IsVisible() const
{
	return GetVisibility() != ESlateVisibility::Collapsed;
}

void UMKUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LocalPlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(LocalPlayerCharacter))
	{
		OwnerASC = LocalPlayerCharacter->GetAbilitySystemComponent();
	}

	ApplyVisualStyle();
	BindEvents();
	UpdateWidget();
}

void UMKUserWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindEvents();
}

void UMKUserWidget::ApplyVisualStyle()
{
	const UDataManager* DataManager = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(DataManager) ? DataManager->GetVisualStyleDataAsset() : nullptr;
	if (::IsValid(WidgetTree) == false || ::IsValid(Style) == false)
	{
		return;
	}

	WidgetTree->ForEachWidget([this, Style](UWidget* Widget)
	{
		if (USpinBox* Input = Cast<USpinBox>(Widget))
		{
			const FSlateBrush* Background = Style->Brushes.Find(TEXT("Button"));
			const FSlateBrush* Hover = Style->Brushes.Find(TEXT("ButtonHover"));
			if (Background && Hover)
			{
				FSpinBoxStyle InputStyle = Input->GetWidgetStyle();
				InputStyle.SetBackgroundBrush(*Background).SetHoveredBackgroundBrush(*Hover);
				InputStyle.SetActiveFillBrush(FSlateNoResource()).SetInactiveFillBrush(FSlateNoResource());
				Input->SetWidgetStyle(InputStyle);
				Input->SetForegroundColor(FSlateColor(FLinearColor(0.96f, 0.9f, 0.73f)));
				FSlateFontInfo Font = Input->GetFont();
				Font.Size = 16;
				Input->SetFont(Font);
			}
		}
		if (UScrollBox* Scroll = Cast<UScrollBox>(Widget))
		{
			Scroll->SetAnimateWheelScrolling(true);
			Scroll->SetScrollAnimationInterpolationSpeed(12.f);
			Scroll->SetAllowOverscroll(false);
			if (const FSlateBrush* Thumb = Style->Brushes.Find(TEXT("ScrollHandle")))
			{
				FScrollBarStyle Bar = Scroll->GetWidgetBarStyle();
				const FSlateRoundedBoxBrush Rail(FLinearColor(0.018f, 0.025f, 0.03f), 2.f, FLinearColor(0.18f, 0.14f, 0.08f), 1.f);
				FSlateBrush HoveredThumb = *Thumb;
				HoveredThumb.TintColor = FSlateColor(FLinearColor(1.15f, 1.08f, 0.9f));
				Bar.SetVerticalBackgroundImage(Rail).SetHorizontalBackgroundImage(Rail);
				Bar.SetVerticalTopSlotImage(FSlateNoResource()).SetVerticalBottomSlotImage(FSlateNoResource());
				Bar.SetHorizontalTopSlotImage(FSlateNoResource()).SetHorizontalBottomSlotImage(FSlateNoResource());
				Bar.SetNormalThumbImage(*Thumb).SetHoveredThumbImage(HoveredThumb).SetDraggedThumbImage(HoveredThumb);
				Scroll->SetWidgetBarStyle(Bar);
				Scroll->SetAlwaysShowScrollbar(false);
				Scroll->SetAlwaysShowScrollbarTrack(false);
				Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
				Scroll->SetScrollbarThickness(FVector2D(12.f, 12.f));
				Scroll->SetScrollbarPadding(FMargin(8.f, 2.f, 0.f, 2.f));
			}
		}
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			// Named surfaces opt into a role; decorative or transparent borders keep their authored style.
			FName BrushName = Border->GetFName();
			if (BrushName == TEXT("MinimapFrame"))
			{
				BrushName = TEXT("Minimap");
			}
			else if (BrushName == TEXT("SkillTreeWindowTitle"))
			{
				BrushName = TEXT("WindowTitle");
			}
			else if (BrushName == TEXT("CurrencyPanel"))
			{
				BrushName = TEXT("InsetPanel");
			}
			ApplyBorderStyle(Border, BrushName);
		}
		if (UImage* Image = Cast<UImage>(Widget))
		{
			const FName BrushName = Image->GetFName() == TEXT("BG") ? TEXT("Panel") : Image->GetFName();
			if (const FSlateBrush* Brush = Style->Brushes.Find(BrushName))
			{
				Image->SetBrush(*Brush);
			}
		}
		if (UButton* Button = Cast<UMKMockupButton>(Widget))
		{
			const FSlateBrush* Normal = Style->Brushes.Find(TEXT("Button"));
			const FSlateBrush* Hover = Style->Brushes.Find(TEXT("ButtonHover"));
			const FSlateBrush* Pressed = Style->Brushes.Find(TEXT("ButtonPressed"));
			const FSlateBrush* Disabled = Style->Brushes.Find(TEXT("ButtonDisabled"));
			if (Normal && Hover && Pressed && Disabled)
			{
				FButtonStyle ButtonStyle = Button->GetStyle();
				ButtonStyle.SetNormal(*Normal).SetHovered(*Hover).SetPressed(*Pressed).SetDisabled(*Disabled);
				ButtonStyle.SetNormalPadding(FMargin(12.f, 8.f)).SetPressedPadding(FMargin(12.f, 9.f, 12.f, 7.f));
				Button->SetStyle(ButtonStyle);
				Button->SetBackgroundColor(FLinearColor::White);
			}
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			const FString WidgetName = TextBlock->GetName();
			if (WidgetName == TEXT("Text_Count") || WidgetName == TEXT("KeyText") || WidgetName == TEXT("CountText"))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Caption);
			}
			else if (WidgetName == TEXT("Text_Coin"))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Numeric);
			}
			else if (WidgetName == TEXT("Title") || WidgetName.EndsWith(TEXT("TitleText")))
			{
				ApplyTextRole(TextBlock, EMKTextRole::SectionTitle);
			}
			else if (WidgetName == TEXT("Desc"))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Body, true);
			}
			else if (WidgetName.EndsWith(TEXT("ButtonText")))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Button);
			}
			else if (WidgetName == TEXT("RecipeNameText") || WidgetName == TEXT("ItemNameText"))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Label, true);
			}
			else if (WidgetName == TEXT("RequiredItemsText") || WidgetName == TEXT("ItemPriceText")
				|| WidgetName == TEXT("UnlockPriceText"))
			{
				ApplyTextRole(TextBlock, EMKTextRole::Caption, true);
			}
		}
	});
}

void UMKUserWidget::ApplyBorderStyle(UBorder* Border, FName BrushName) const
{
	if (BrushName == TEXT("RecipeCard"))
	{
		BrushName = TEXT("InsetPanel");
	}

	const UDataManager* DataManager = UDataManager::Get(const_cast<UMKUserWidget*>(this));
	const UVisualStyleDataAsset* Style = ::IsValid(DataManager) ? DataManager->GetVisualStyleDataAsset() : nullptr;
	const FSlateBrush* Brush = ::IsValid(Style) ? Style->Brushes.Find(BrushName) : nullptr;
	if (::IsValid(Border) && Brush)
	{
		Border->SetBrush(*Brush);
		Border->SetBrushColor(FLinearColor::White);
	}
}

void UMKUserWidget::ApplyTextRole(UTextBlock* TextBlock, const EMKTextRole TextRole, const bool bAutoWrap) const
{
	if (::IsValid(TextBlock) == false)
	{
		return;
	}

	FSlateFontInfo Font = TextBlock->GetFont();
	switch (TextRole)
	{
	case EMKTextRole::Caption:
		Font.Size = 12;
		Font.TypefaceFontName = TEXT("Regular");
		break;
	case EMKTextRole::Body:
		Font.Size = 14;
		Font.TypefaceFontName = TEXT("Regular");
		break;
	case EMKTextRole::Label:
		Font.Size = 16;
		Font.TypefaceFontName = TEXT("Bold");
		break;
	case EMKTextRole::Button:
		Font.Size = 14;
		Font.TypefaceFontName = TEXT("Bold");
		TextBlock->SetJustification(ETextJustify::Center);
		break;
	case EMKTextRole::SectionTitle:
		Font.Size = 18;
		Font.TypefaceFontName = TEXT("Bold");
		break;
	case EMKTextRole::WindowTitle:
		Font.Size = 22;
		Font.TypefaceFontName = TEXT("Bold");
		break;
	case EMKTextRole::Numeric:
		Font.Size = 16;
		Font.TypefaceFontName = TEXT("Bold");
		break;
	case EMKTextRole::HUDMetric:
	{
		static const TSharedPtr<const FCompositeFont> MetricFont = MakeShared<FCompositeFont>(TEXT("BoldCondensed"),
			FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-BoldCondensed.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		Font = FSlateFontInfo(MetricFont, 14, TEXT("BoldCondensed"));
		Font.LetterSpacing = 80;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.015f, 0.025f, 0.03f);
		break;
	}
	default:
		break;
	}

	TextBlock->SetFont(Font);
	TextBlock->SetAutoWrapText(bAutoWrap);
	TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
	TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
}

UBorder* UMKUserWidget::BuildSectionHeader(const FText& Title, FName IconBrushName, UTextBlock** OutTitle)
{
	UBorder* Header = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WindowTitle"));
	Header->SetPadding(FMargin(FMKUILayout::SpaceXL + FMKUILayout::SpaceS, FMKUILayout::SpaceS, FMKUILayout::SpaceM, FMKUILayout::SpaceS));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Header->SetContent(Row);
	if (IconBrushName.IsNone() == false)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconBrushName);
		Icon->SetDesiredSizeOverride(FVector2D(24.f));
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceS, 0.f));
	}
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(Title);
	ApplyTextRole(Label, EMKTextRole::WindowTitle);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.87f, 0.68f)));
	Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
	if (OutTitle != nullptr) { *OutTitle = Label; }
	return Header;
}

UTextBlock* UMKUserWidget::BuildSectionLabel(const FText& Label)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
	TextBlock->SetText(Label);
	ApplyTextRole(TextBlock, EMKTextRole::SectionTitle);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.88f, 0.8f)));
	return TextBlock;
}

USizeBox* UMKUserWidget::BuildSectionDivider()
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(12.f);
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Size->SetContent(Overlay);
	USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>();
	LineSize->SetHeightOverride(1.f);
	UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
	Line->SetBrush(FSlateColorBrush(FLinearColor(0.32f, 0.25f, 0.16f)));
	Line->SetPadding(FMargin(0.f));
	LineSize->SetContent(Line);
	UOverlaySlot* Layout = Overlay->AddChildToOverlay(LineSize);
	Layout->SetHorizontalAlignment(HAlign_Fill);
	Layout->SetVerticalAlignment(VAlign_Center);
	Layout->SetPadding(FMargin(8.f, 0.f));
	return Size;
}

UBorder* UMKUserWidget::BuildItemIconFrame(UImage*& OutIcon, const float IconSize, const FName BrushName)
{
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BrushName);
	ApplyBorderStyle(Frame, BrushName);
	Frame->SetPadding(FMargin(FMKUILayout::SpaceS));
	OutIcon = WidgetTree->ConstructWidget<UImage>();
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(IconSize);
	Size->SetHeightOverride(IconSize);
	Size->SetContent(OutIcon);
	Frame->SetContent(Size);
	return Frame;
}

USizeBox* UMKUserWidget::BuildStatusGauge(UProgressBar*& OutBar, FName RootName, FName FrameName, const FText& Label, const FLinearColor& FillColor)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), RootName);
	Size->SetWidthOverride(260.f);
	Size->SetHeightOverride(32.f);
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FrameName);
	Frame->SetPadding(FMargin(34.f, 8.f, 7.f, 9.f));
	Size->SetContent(Frame);
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Frame->SetContent(Overlay);
	OutBar = WidgetTree->ConstructWidget<UProgressBar>();
	FProgressBarStyle Style;
	Style.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.055f, 0.06f, 0.07f)));
	Style.SetFillImage(FSlateColorBrush(FLinearColor::White));
	const UDataManager* Data = UDataManager::Get(this);
	const UVisualStyleDataAsset* VisualStyle = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
	const FSlateBrush* FillBrush = ::IsValid(VisualStyle) ? VisualStyle->Brushes.Find(FrameName == TEXT("OxygenFrame") ? TEXT("OxygenFill") : TEXT("HealthFill")) : nullptr;
	if (FillBrush != nullptr) { Style.SetFillImage(*FillBrush); }
	OutBar->SetWidgetStyle(Style);
	OutBar->SetBorderPadding(FVector2D::ZeroVector);
	OutBar->SetFillColorAndOpacity(FillBrush != nullptr ? FLinearColor::White : FillColor);
	UOverlaySlot* Fill = Overlay->AddChildToOverlay(OutBar);
	Fill->SetHorizontalAlignment(HAlign_Fill);
	Fill->SetVerticalAlignment(VAlign_Fill);
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(Label);
	ApplyTextRole(Text, EMKTextRole::HUDMetric);
	FSlateFontInfo GaugeFont = Text->GetFont();
	GaugeFont.Size = 12;
	Text->SetFont(GaugeFont);
	Text->SetShadowOffset(FVector2D::ZeroVector);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	Fit->SetStretchDirection(EStretchDirection::DownOnly);
	Fit->SetClipping(EWidgetClipping::ClipToBounds);
	Fit->SetContent(Text);
	CastChecked<UScaleBoxSlot>(Text->Slot)->SetHorizontalAlignment(HAlign_Left);
	UOverlaySlot* Copy = Overlay->AddChildToOverlay(Fit);
	Copy->SetHorizontalAlignment(HAlign_Fill);
	Copy->SetVerticalAlignment(VAlign_Fill);
	Copy->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
	return Size;
}

USizeBox* UMKUserWidget::BuildActionButton(UButton*& OutButton, UTextBlock*& OutLabel, const FText& Label,
	const float MinWidth, const float MinHeight)
{
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
	ButtonSize->SetMinDesiredWidth(MinWidth);
	ButtonSize->SetMinDesiredHeight(MinHeight);
	OutButton = WidgetTree->ConstructWidget<UMKMockupButton>();
	OutLabel = WidgetTree->ConstructWidget<UTextBlock>();
	OutLabel->SetText(Label);
	ApplyTextRole(OutLabel, EMKTextRole::Button);
	OutButton->SetContent(OutLabel);
	ButtonSize->SetContent(OutButton);
	return ButtonSize;
}

UHorizontalBox* UMKUserWidget::BuildCurrencyAmount(UTextBlock*& OutAmountText, const FText& Amount, const EMKTextRole TextRole)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	UImage* CurrencyIcon = WidgetTree->ConstructWidget<UImage>();
	const UDataManager* DataManager = UDataManager::Get(this);
	const UVisualStyleDataAsset* Style = ::IsValid(DataManager) ? DataManager->GetVisualStyleDataAsset() : nullptr;
	if (const FSlateBrush* CurrencyBrush = ::IsValid(Style) ? Style->Brushes.Find(TEXT("Currency")) : nullptr)
	{
		CurrencyIcon->SetBrush(*CurrencyBrush);
	}
	USizeBox* CurrencySize = WidgetTree->ConstructWidget<USizeBox>();
	CurrencySize->SetWidthOverride(20.f);
	CurrencySize->SetHeightOverride(20.f);
	CurrencySize->SetContent(CurrencyIcon);
	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(CurrencySize);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.f, 0.f, FMKUILayout::SpaceXS, 0.f));

	OutAmountText = WidgetTree->ConstructWidget<UTextBlock>();
	OutAmountText->SetText(Amount);
	ApplyTextRole(OutAmountText, TextRole);
	Row->AddChildToHorizontalBox(OutAmountText)->SetVerticalAlignment(VAlign_Center);
	return Row;
}

USizeBox* UMKUserWidget::BuildCurrencyButton(UButton*& OutButton, UTextBlock*& OutAmountText, const FText& Amount,
	const float MinWidth, const float MinHeight)
{
	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
	ButtonSize->SetMinDesiredWidth(MinWidth);
	ButtonSize->SetMinDesiredHeight(MinHeight);
	OutButton = WidgetTree->ConstructWidget<UMKMockupButton>();
	OutButton->SetContent(BuildCurrencyAmount(OutAmountText, Amount, EMKTextRole::Button));
	ButtonSize->SetContent(OutButton);
	return ButtonSize;
}

AMKCharacter* UMKUserWidget::GetLocalPlayerCharacter() const
{
	return LocalPlayerCharacter.Get();
}

UHUDWidget* UMKUserWidget::GetHUDWidget() const
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<UMKUserWidget*>(this), FoundWidgets, UHUDWidget::StaticClass(), false);
	return FoundWidgets.Num() > 0 ? Cast<UHUDWidget>(FoundWidgets[0]) : nullptr;
}

UAbilitySystemComponent* UMKUserWidget::GetOwnerAbilitySystemComponent() const
{
	return OwnerASC.Get();
}

UInventoryComponent* UMKUserWidget::GetLocalInventoryComponent() const
{
	const AMKCharacter* PlayerCharacter = LocalPlayerCharacter.Get();
	if (::IsValid(PlayerCharacter) == false)
	{
		PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	return ::IsValid(PlayerCharacter) ? PlayerCharacter->GetComponentByClass<UInventoryComponent>() : nullptr;
}

USkillComponent* UMKUserWidget::GetLocalSkillComponent() const
{
	const AMKCharacter* PlayerCharacter = LocalPlayerCharacter.Get();
	if (::IsValid(PlayerCharacter) == false)
	{
		PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	return ::IsValid(PlayerCharacter) ? PlayerCharacter->GetComponentByClass<USkillComponent>() : nullptr;
}

const UAttributeSet_Character* UMKUserWidget::GetCharacterAttributeSet() const
{
	const UAbilitySystemComponent* AbilitySystemComponent = OwnerASC.Get();
	if (::IsValid(AbilitySystemComponent) == false)
	{
		const AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		AbilitySystemComponent = ::IsValid(PlayerCharacter) ? PlayerCharacter->GetAbilitySystemComponent() : nullptr;
	}

	return ::IsValid(AbilitySystemComponent)
		? Cast<UAttributeSet_Character>(AbilitySystemComponent->GetAttributeSet(UAttributeSet_Character::StaticClass())) : nullptr;
}

void UMKUserWidget::SetVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
