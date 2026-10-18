#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "PlaytestWidget.generated.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class AMKMonster;

// PIE 전용 목업. 정식 HUD와 테스트 도구가 완성되면 Controller의 생성 경로와 함께 제거한다.
UCLASS()
class PROJECTMK_API UPlaytestWidget : public UMKUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

public:
	void TogglePanel();

private:
	void BuildDefaultWidgetTree();
	UButton* AddActionButton(UHorizontalBox* Row, const FText& Label, const FText& Hint);
	void UpdateStatus();
	bool TryStartPlaytestSession();
	void ApplyTestInventory(bool bConsumables);

	UFUNCTION()
	void OnTogglePanelClicked();

	UFUNCTION()
	void OnMaterialsClicked();

	UFUNCTION()
	void OnConsumablesClicked();

	UFUNCTION()
	void OnCoinsClicked();

	UFUNCTION()
	void OnShopClicked();

	UFUNCTION()
	void OnCraftingClicked();

	UFUNCTION()
	void OnSkillsClicked();

	UFUNCTION()
	void OnDamageClicked();

	UFUNCTION()
	void OnDrainOxygenClicked();

	UFUNCTION()
	void OnRestoreClicked();

	UFUNCTION()
	void OnReturnClicked();

	UFUNCTION()
	void OnSpawnMonsterClicked();

	UFUNCTION()
	void OnHitMonsterClicked();

	UPROPERTY(EditDefaultsOnly, Category = "Playtest", meta = (GetOptions = "ProjectMK.MKBlueprintFunctionLibrary.GetMonsterRowNames"))
	FName MonsterRowKey = TEXT("DrillCrawler");

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DetailsPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TogglePanelButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MaterialsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConsumablesButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CoinsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ShopButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CraftingButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SkillsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DamageButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DrainOxygenButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestoreButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ReturnButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SpawnMonsterButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> HitMonsterButton;

	TWeakObjectPtr<AMKMonster> TestMonster;

	FTimerHandle StatusTimerHandle;
};
