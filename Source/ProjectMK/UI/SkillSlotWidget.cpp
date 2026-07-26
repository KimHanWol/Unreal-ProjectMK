// LINK

#include "ProjectMK/UI/SkillSlotWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"

void USkillSlotWidget::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).AddUObject(this, &USkillSlotWidget::OnCoinChanged);
	}
}

void USkillSlotWidget::UnbindEvents()
{
	Super::UnbindEvents();

	if (::IsValid(OwnerASC))
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UAttributeSet_Character::GetCoinAttribute()).RemoveAll(this);
	}
}

void USkillSlotWidget::UpdateWidget()
{
	UpdateSkillImage();
	UpdateCompleteImage();
}

bool USkillSlotWidget::CanClickSlot() const
{
	if (SkillKeyName.IsNone())
	{
		return false;
	}

	UDataManager* DataManager = UDataManager::Get(const_cast<USkillSlotWidget*>(this));
	return ::IsValid(DataManager) && DataManager->GetSkillDataTableRow(SkillKeyName) != nullptr;
}

void USkillSlotWidget::OnSlotClicked()
{
	USkillComponent* SkillComponent = GetLocalSkillComponent();
	if (::IsValid(SkillComponent) == false)
	{
		return;
	}

	if (SkillComponent->TryPurchaseSkill(SkillKeyName))
	{
		UpdateCompleteImage();
	}
}

void USkillSlotWidget::UpdateCompleteImage()
{
	if (::IsValid(CompleteImage) == false)
	{
		return;
	}

	USkillComponent* SkillComponent = GetLocalSkillComponent();
	const bool bIsPurchased = ::IsValid(SkillComponent) && SkillComponent->HasPurchasedSkill(SkillKeyName);
	CompleteImage->SetVisibility(bIsPurchased ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void USkillSlotWidget::UpdateSkillImage()
{
	if (::IsValid(SkillImage) == false)
	{
		return;
	}

	SkillImage->SetBrush(FSlateBrush());

	if (SkillKeyName.IsNone())
	{
		return;
	}

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillKeyName);
	if (SkillDataTableRow == nullptr || SkillDataTableRow->IconPath.IsNull())
	{
		return;
	}

	UTexture2D* IconTexture = SkillDataTableRow->IconPath.LoadSynchronous();
	if (::IsValid(IconTexture) == false)
	{
		return;
	}

	SkillImage->SetBrushFromTexture(IconTexture, true);
}

void USkillSlotWidget::OnCoinChanged(const FOnAttributeChangeData& Data)
{
	UpdateWidget();
}
