#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "MKUserWidget.generated.h"

class AMKCharacter;
class UAbilitySystemComponent;
class UAttributeSet_Character;
class UHUDWidget;
class UInventoryComponent;
class USkillComponent;
class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class USizeBox;
class UTextBlock;

enum class EMKTextRole : uint8
{
	Caption,
	Body,
	Label,
	Button,
	SectionTitle,
	WindowTitle,
	Numeric,
};

struct FMKUILayout
{
	static constexpr float SpaceXS = 4.f;
	static constexpr float SpaceS = 8.f;
	static constexpr float SpaceM = 12.f;
	static constexpr float SpaceL = 16.f;
	static constexpr float SpaceXL = 24.f;
	static constexpr float ItemIconSize = 40.f;
	static constexpr float CompactButtonWidth = 112.f;
	static constexpr float ButtonHeight = 44.f;
};

UCLASS()
class PROJECTMK_API UMKUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool IsVisible() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void BindEvents() {}
	virtual void UnbindEvents() {}
	virtual void UpdateWidget() {}

	void ApplyVisualStyle();
	void ApplyBorderStyle(UBorder* Border, FName BrushName) const;
	void ApplyTextRole(UTextBlock* TextBlock, EMKTextRole TextRole, bool bAutoWrap = false) const;
	UBorder* BuildSectionHeader(const FText& Title, FName IconBrushName);
	UTextBlock* BuildSectionLabel(const FText& Label);
	UBorder* BuildItemIconFrame(UImage*& OutIcon, float IconSize = FMKUILayout::ItemIconSize, FName BrushName = TEXT("Slot"));
	USizeBox* BuildActionButton(UButton*& OutButton, UTextBlock*& OutLabel, const FText& Label,
		float MinWidth = FMKUILayout::CompactButtonWidth, float MinHeight = FMKUILayout::ButtonHeight);
	UHorizontalBox* BuildCurrencyAmount(UTextBlock*& OutAmountText, const FText& Amount, EMKTextRole TextRole = EMKTextRole::Numeric);
	USizeBox* BuildCurrencyButton(UButton*& OutButton, UTextBlock*& OutAmountText, const FText& Amount,
		float MinWidth = FMKUILayout::CompactButtonWidth, float MinHeight = FMKUILayout::ButtonHeight);

	AMKCharacter* GetLocalPlayerCharacter() const;
	UHUDWidget* GetHUDWidget() const;

	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	UInventoryComponent* GetLocalInventoryComponent() const;
	USkillComponent* GetLocalSkillComponent() const;
	const UAttributeSet_Character* GetCharacterAttributeSet() const;
	void SetVisible(bool bVisible);

protected:
	UPROPERTY(Transient)
	TObjectPtr<AMKCharacter> LocalPlayerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> OwnerASC;
};
