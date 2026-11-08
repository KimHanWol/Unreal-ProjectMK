#include "ProjectMK/UI/MKMockupButton.h"

#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

void UMKMockupButton::SetVisualRole(FName Role)
{
	VisualRole = Role;
	ApplyButtonStyle();
}

void UMKMockupButton::ApplyButtonStyle()
{
	const UDataManager* Data = UDataManager::Get(this);
	const UVisualStyleDataAsset* VisualStyle = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
	if (::IsValid(VisualStyle) == false) { return; }
	const FSlateBrush* Source = VisualStyle->Brushes.Find(VisualRole);
	if (Source == nullptr) { Source = VisualStyle->Brushes.Find(TEXT("Button")); }
	if (Source == nullptr) { return; }
	FSlateBrush Hover = *Source;
	if (VisualRole == TEXT("QuantityPreset"))
	{
		if (const FSlateBrush* Selected = VisualStyle->Brushes.Find(TEXT("QuantityPresetSelected"))) { Hover = *Selected; }
	}
	Hover.TintColor = FSlateColor(FLinearColor(1.15f, 1.12f, 1.04f));
	FSlateBrush Pressed = *Source;
	Pressed.TintColor = FSlateColor(FLinearColor(0.68f, 0.72f, 0.74f));
	FSlateBrush Disabled = *Source;
	Disabled.TintColor = FSlateColor(FLinearColor(0.38f, 0.4f, 0.42f, 0.8f));
	FButtonStyle ButtonStyle = GetStyle();
	ButtonStyle.SetNormal(*Source).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Disabled);
	const FMargin Padding = VisualRole == TEXT("Stepper") ? FMargin(4.f) : FMargin(12.f, 6.f);
	ButtonStyle.SetNormalPadding(Padding).SetPressedPadding(Padding + FMargin(0.f, 1.f, 0.f, -1.f));
	SetStyle(ButtonStyle);
	SetBackgroundColor(FLinearColor::White);
}
