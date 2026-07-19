#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "MKUserWidget.generated.h"

class AMKCharacter;
class UAbilitySystemComponent;
class UAttributeSet_Character;
class UInventoryComponent;

UCLASS()
class PROJECTMK_API UMKUserWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void BindEvents() {}
	virtual void UnbindEvents() {}

	AMKCharacter* GetLocalPlayerCharacter() const { return LocalPlayerCharacter.Get(); }
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const { return OwnerASC.Get(); }
	UInventoryComponent* GetLocalInventoryComponent() const;
	const UAttributeSet_Character* GetCharacterAttributeSet() const;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AMKCharacter> LocalPlayerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> OwnerASC;
};
