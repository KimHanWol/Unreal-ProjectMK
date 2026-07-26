#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "ProjectMK/UI/MKUserWidget.h"

#include "MKMenuBase.generated.h"

class UButton;
class UCanvasPanel;
class UMKMenuContentsBase;

UCLASS()
class PROJECTMK_API UMKMenuBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	bool OpenMenu(EMenuContentsType InMenuContentsType);

	UFUNCTION(BlueprintCallable)
	void CloseMenu();

	bool IsMenuOpen() const;

protected:
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	virtual void UpdateWidget() override;

private:
	UFUNCTION()
	void OnExitButtonClicked();

	void CacheMenuContents();
	bool UpdateMenuContents(EMenuContentsType InMenuContentsType);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MenuContentsCanvasPanel;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMKMenuContentsBase> CurrentMenuContents;

	UPROPERTY(Transient)
	TMap<EMenuContentsType, TObjectPtr<UMKMenuContentsBase>> MenuContentsMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Menu", meta = (AllowPrivateAccess = "true"))
	EMenuContentsType CurrentMenuContentsType = EMenuContentsType::None;
};
