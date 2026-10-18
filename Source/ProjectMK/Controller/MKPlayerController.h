// LINK

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MKPlayerController.generated.h"

class UHUDWidget;
class UMinimapWidget;
class UPlaytestWidget;
class UQuickInventoryWidget;

UCLASS()
class PROJECTMK_API AMKPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMKPlayerController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	void ToggleInventoryWidget();
	void ToggleSkillTree();
	void ToggleCrafting();
	bool TryOpenShop();
	void TogglePlaytestPanel();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMinimapWidget> MinimapWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Mockup")
	TSubclassOf<UQuickInventoryWidget> QuickInventoryWidgetClass;

private:
	void CreateMinimapWidget();
	void CreatePlaytestWidget();
	UHUDWidget* GetHUDWidget();

	UPROPERTY(Transient)
	TObjectPtr<UPlaytestWidget> PlaytestWidget;

	UPROPERTY(Transient)
	TObjectPtr<UQuickInventoryWidget> QuickInventoryWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMinimapWidget> MinimapWidget;
};
