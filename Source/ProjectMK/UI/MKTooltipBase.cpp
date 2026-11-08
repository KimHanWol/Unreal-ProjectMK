// LINK

#include "ProjectMK/UI/MKTooltipBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/SkillDataTableRow.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/SkillComponent.h"
#include "ProjectMK/AbilitySystem/AttributeSet/AttributeSet_Character.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"

namespace
{
	const FVector2D TooltipItemIconBrushSize(64.f, 64.f);

	bool TrySetBrushFromTextureRegion(UImage* TargetImage, UTexture2D* Texture, const FVector2D& SourceUV, const FVector2D& SourceSize)
	{
		if (::IsValid(TargetImage) == false || ::IsValid(Texture) == false)
		{
			return false;
		}

		const float TextureWidth = static_cast<float>(Texture->GetSizeX());
		const float TextureHeight = static_cast<float>(Texture->GetSizeY());
		if (TextureWidth <= 0.f || TextureHeight <= 0.f)
		{
			return false;
		}

		const FVector2D MinUV(SourceUV.X / TextureWidth, SourceUV.Y / TextureHeight);
		const FVector2D MaxUV((SourceUV.X + SourceSize.X) / TextureWidth, (SourceUV.Y + SourceSize.Y) / TextureHeight);

		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = TooltipItemIconBrushSize;
		Brush.SetUVRegion(FBox2D(MinUV, MaxUV));

		TargetImage->SetBrush(Brush);
		TargetImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return true;
	}
}

TSharedRef<SWidget> UMKTooltipBase::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->FindWidget(TEXT("RuntimeTooltipRoot")) == nullptr)
	{
		BuildDefaultWidgetTree();
	}

	return Super::RebuildWidget();
}

void UMKTooltipBase::ShowItemTooltip(FName ItemDataKey)
{
	TakeWidget();
	ClearTooltip();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false || ItemDataKey.IsNone())
	{
		SetVisible(false);
		return;
	}

	const FItemDataTableRow* ItemDataTableRow = DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemDataKey);
	if (ItemDataTableRow == nullptr)
	{
		SetVisible(false);
		return;
	}

	UpdateTooltipFromItemData(ItemDataKey, *ItemDataTableRow);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMKTooltipBase::ShowSkillTooltip(FName SkillDataKey)
{
	TakeWidget();
	ClearTooltip();

	UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false || SkillDataKey.IsNone())
	{
		SetVisible(false);
		return;
	}

	const FSkillDataTableRow* SkillDataTableRow = DataManager->GetSkillDataTableRow(SkillDataKey);
	if (SkillDataTableRow == nullptr)
	{
		SetVisible(false);
		return;
	}

	UpdateTooltipFromSkillData(SkillDataKey, *SkillDataTableRow);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMKTooltipBase::ShowRecipeTooltip(FName RecipeKey, bool bBlueprint)
{
	TakeWidget();
	ClearTooltip();
	const UDataManager* Data = UDataManager::Get(this);
	const FShopRecipeDataTableRow* Recipe = ::IsValid(Data) ? Data->GetShopRecipeDataTableRow(RecipeKey) : nullptr;
	if (Recipe == nullptr) { SetVisible(false); return; }
	Title->SetText(Data->GetRecipeDisplayName(RecipeKey, bBlueprint));
	CategoryText->SetText(bBlueprint ? NSLOCTEXT("Tooltip", "Blueprint", "설계도") : NSLOCTEXT("Tooltip", "CraftingRecipe", "제작"));
	const FItemDataTableRow* Result = Data->GetRecipeItemData(RecipeKey, bBlueprint);
	const FText Description = Data->GetRecipeDescription(RecipeKey, bBlueprint);
	Desc->SetText(Description);
	Desc->SetVisibility(Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (Result != nullptr && Result->ItemIcon.IsNull() == false) { UpdateTooltipIcon(Result->ItemIcon.LoadSynchronous()); }
	UInventoryComponent* Inventory = GetLocalInventoryComponent();
	bool bEnoughMaterials = ::IsValid(Inventory);
	for (const FShopRecipeItem& Required : Recipe->RequiredItems)
	{
		const FName Key = Required.GetItemKey();
		const int32 Owned = ::IsValid(Inventory) ? Inventory->GetItemCount(Key) : 0;
		bEnoughMaterials &= Owned >= Required.ItemCount;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		MaterialRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 4.f));
		UImage* MaterialIcon = nullptr;
		Row->AddChildToHorizontalBox(BuildItemIconFrame(MaterialIcon, 20.f))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		const FItemDataTableRow* Item = Data->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, Key);
		if (Item != nullptr) { MaterialIcon->SetBrushResourceObject(Item->ItemIcon.LoadSynchronous()); }
		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(Data->GetItemDisplayName(Key));
		ApplyTextRole(Name, EMKTextRole::Body);
		UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Count = WidgetTree->ConstructWidget<UTextBlock>();
		ApplyTextRole(Count, EMKTextRole::Numeric);
		Count->SetText(FText::Format(NSLOCTEXT("Tooltip", "MaterialCount", "{0} / {1}"), FText::AsNumber(Owned), FText::AsNumber(Required.ItemCount)));
		Count->SetColorAndOpacity(FSlateColor(Owned >= Required.ItemCount ? FLinearColor(0.55f, 0.85f, 0.65f) : FLinearColor(0.95f, 0.56f, 0.4f)));
		Row->AddChildToHorizontalBox(Count)->SetVerticalAlignment(VAlign_Center);
	}
	AMKCharacter* Character = GetLocalPlayerCharacter();
	UCraftingComponent* Crafting = ::IsValid(Character) ? Character->GetCraftingComponent() : nullptr;
	const bool bUnlocked = ::IsValid(Crafting) && Crafting->IsRecipeUnlocked(RecipeKey);
	SetCost(Recipe->UnlockPrice, bBlueprint && bUnlocked == false);
	FText State;
	if (bBlueprint)
	{
		const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
		State = bUnlocked ? NSLOCTEXT("Tooltip", "AlreadyPurchased", "구매 완료")
			: (::IsValid(Attributes) && Attributes->GetCoin() < Recipe->UnlockPrice
				? NSLOCTEXT("Tooltip", "NoCoin", "코인 부족") : NSLOCTEXT("Tooltip", "Purchasable", "구매 가능"));
	}
	else if (bUnlocked)
	{
		State = Crafting->CanCraftRecipe(RecipeKey) ? NSLOCTEXT("Tooltip", "Craftable", "제작 가능")
			: (bEnoughMaterials ? NSLOCTEXT("Tooltip", "NoRoom", "공간 부족") : NSLOCTEXT("Tooltip", "NoMaterial", "재료 부족"));
	}
	else
	{
		const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
		State = ::IsValid(Attributes) && Attributes->GetCoin() < Recipe->UnlockPrice ? NSLOCTEXT("Tooltip", "NoCoin", "코인 부족") : NSLOCTEXT("Tooltip", "NeedUnlock", "해금 필요");
	}
	SetStatus(State, FLinearColor(0.8f, 0.72f, 0.52f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMKTooltipBase::HideTooltip()
{
	ClearTooltip();
	SetVisible(false);
}

void UMKTooltipBase::UpdateWidget()
{
	ClearTooltip();
	SetVisible(false);
}

void UMKTooltipBase::BuildDefaultWidgetTree()
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RuntimeTooltipRoot"));
	Size->SetWidthOverride(360.f);
	Size->SetMinDesiredHeight(112.f);
	WidgetTree->RootWidget = Size;
	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Tooltip"));
	Frame->SetPadding(FMargin(24.f));
	Size->SetContent(Frame);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Column);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	UImage* Image = nullptr;
	UBorder* IconFrame = BuildItemIconFrame(Image, 40.f);
	Icon = Image;
	Header->AddChildToHorizontalBox(IconFrame)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	UVerticalBox* Heading = WidgetTree->ConstructWidget<UVerticalBox>();
	UHorizontalBoxSlot* HeadingSlot = Header->AddChildToHorizontalBox(Heading);
	HeadingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeadingSlot->SetVerticalAlignment(VAlign_Center);
	Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RuntimeTooltipTitle"));
	ApplyTextRole(Title, EMKTextRole::SectionTitle);
	Title->SetWrapTextAt(240.f);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.86f, 0.56f)));
	Heading->AddChildToVerticalBox(Title);
	CategoryText = WidgetTree->ConstructWidget<UTextBlock>();
	ApplyTextRole(CategoryText, EMKTextRole::Caption);
	CategoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.7f, 0.73f)));
	Heading->AddChildToVerticalBox(CategoryText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RuntimeTooltipDescription"));
	ApplyTextRole(Desc, EMKTextRole::Body);
	Desc->SetWrapTextAt(312.f);
	Desc->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.86f, 0.82f)));
	Column->AddChildToVerticalBox(Desc)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	MaterialRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TooltipMaterials"));
	Column->AddChildToVerticalBox(MaterialRows);
	UTextBlock* Price = nullptr;
	CostRow = BuildCurrencyAmount(Price, FText::GetEmpty());
	CostText = Price;
	Column->AddChildToVerticalBox(CostRow)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TooltipStatus"));
	ApplyTextRole(StatusText, EMKTextRole::Body);
	StatusText->SetWrapTextAt(312.f);
	Column->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

void UMKTooltipBase::ClearTooltip()
{
	if (::IsValid(Title)) { Title->SetText(FText::GetEmpty()); }
	if (::IsValid(Desc)) { Desc->SetText(FText::GetEmpty()); Desc->SetVisibility(ESlateVisibility::Collapsed); }
	if (::IsValid(CategoryText)) { CategoryText->SetText(FText::GetEmpty()); }
	if (::IsValid(MaterialRows)) { MaterialRows->ClearChildren(); }
	if (::IsValid(CostRow)) { CostRow->SetVisibility(ESlateVisibility::Collapsed); }
	if (::IsValid(StatusText)) { StatusText->SetVisibility(ESlateVisibility::Collapsed); }
	if (::IsValid(Icon)) { Icon->SetBrush(FSlateBrush()); Icon->SetVisibility(ESlateVisibility::Collapsed); }
}

void UMKTooltipBase::UpdateTooltipFromItemData(FName ItemDataKey, const FItemDataTableRow& Item)
{
	Title->SetText(UDataManager::Get(this)->GetItemDisplayName(ItemDataKey));
	CategoryText->SetText(Item.bIsOre ? NSLOCTEXT("Tooltip", "Ore", "광물") : NSLOCTEXT("Tooltip", "Item", "아이템"));
	Desc->SetText(Item.Description);
	Desc->SetVisibility(Item.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (Item.ItemIcon.IsNull() == false) { UpdateTooltipIcon(Item.ItemIcon.LoadSynchronous()); }
	SetCost(Item.SellPrice, Item.SellPrice > 0);
	if (Item.SellPrice > 0) { SetStatus(NSLOCTEXT("Tooltip", "UnitPrice", "개당 판매가"), FLinearColor(0.62f, 0.7f, 0.7f)); }
}

void UMKTooltipBase::UpdateTooltipFromSkillData(FName SkillKey, const FSkillDataTableRow& Skill)
{
	Title->SetText(Skill.DisplayName);
	CategoryText->SetText(NSLOCTEXT("Tooltip", "Skill", "기술"));
	Desc->SetText(Skill.Description);
	Desc->SetVisibility(Skill.Description.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (Skill.IconPath.IsNull() == false) { UpdateTooltipIcon(Skill.IconPath.LoadSynchronous()); }
	USkillComponent* Skills = GetLocalSkillComponent();
	const bool bPurchased = ::IsValid(Skills) && Skills->HasPurchasedSkill(SkillKey);
	SetCost(Skill.Price, bPurchased == false);
	if (bPurchased) { SetStatus(NSLOCTEXT("Tooltip", "Learned", "습득 완료"), FLinearColor(0.45f, 0.86f, 0.67f)); return; }
	for (FName Required : Skill.RequiredSkill)
	{
		if (::IsValid(Skills) == false || Skills->HasPurchasedSkill(Required) == false)
		{
			SetStatus(NSLOCTEXT("Tooltip", "Prerequisite", "선행 기술 필요"), FLinearColor(0.9f, 0.65f, 0.4f));
			return;
		}
	}
	const UAttributeSet_Character* Attributes = GetCharacterAttributeSet();
	const bool bCanAfford = ::IsValid(Attributes) && Attributes->GetCoin() >= Skill.Price;
	SetStatus(bCanAfford ? NSLOCTEXT("Tooltip", "Hold", "길게 눌러 구매") : NSLOCTEXT("Tooltip", "NoCoin", "코인 부족"),
		bCanAfford ? FLinearColor(0.6f, 0.8f, 0.76f) : FLinearColor(0.95f, 0.5f, 0.38f));
}

void UMKTooltipBase::SetCost(int32 Cost, bool bShow)
{
	CostText->SetText(FText::AsNumber(Cost));
	CostRow->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMKTooltipBase::SetStatus(const FText& Text, const FLinearColor& Color)
{
	StatusText->SetText(Text);
	StatusText->SetColorAndOpacity(FSlateColor(Color));
	StatusText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UMKTooltipBase::UpdateTooltipIcon(UTexture2D* IconTexture)
{
	if (::IsValid(Icon) == false || ::IsValid(IconTexture) == false)
	{
		return;
	}

	Icon->SetBrushFromTexture(IconTexture, true);
	Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UMKTooltipBase::UpdateTooltipIcon(UPaperSprite* IconSprite)
{
	if (::IsValid(IconSprite) == false)
	{
		return;
	}

	TrySetBrushFromTextureRegion(Icon, IconSprite->GetSourceTexture(), IconSprite->GetSourceUV(), IconSprite->GetSourceSize());
}
