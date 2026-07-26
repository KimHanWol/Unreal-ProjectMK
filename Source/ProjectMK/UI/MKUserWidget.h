#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "MKUserWidget.generated.h"

class AMKCharacter;
class UAbilitySystemComponent;
class UAttributeSet_Character;
class UInventoryComponent;
class USkillComponent;

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

	AMKCharacter* GetLocalPlayerCharacter() const;

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
