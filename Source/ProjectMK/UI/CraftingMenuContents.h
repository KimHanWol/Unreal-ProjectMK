#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "CraftingMenuContents.generated.h"

class UCraftingComponent;
class UCraftingRecipeSlotWidget;
class UPanelWidget;
class UTextBlock;

/**
 * 제작 메뉴. 위젯 블루프린트가 없으면 테스트용 기본 레이아웃을 만든다.
 */
UCLASS()
class PROJECTMK_API UCraftingMenuContents : public UMKMenuContentsBase
{
	GENERATED_BODY()

public:
	UCraftingMenuContents(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	/** 해금된(구매했거나 가격이 0 인) 레시피 목록 */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FName> GetUnlockedRecipeKeys() const;

	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraftRecipe(FName RecipeKey) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool TryCraftRecipe(FName RecipeKey);

	/** 인벤토리나 해금 상태가 바뀌어 목록을 다시 그려야 할 때 호출된다 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void BP_OnCraftingRefreshed();

private:
	void BuildDefaultWidgetTree();
	void UpdateRecipeSlots();
	UCraftingComponent* GetCraftingComponent() const;

	UFUNCTION()
	void OnMenuVisibilityChanged(ESlateVisibility InVisibility);

	void OnInventoryChanged();
	void OnRecipeChanged(FName RecipeKey);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> RecipeSlotListPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCraftingRecipeSlotWidget>> RecipeSlots;
};
