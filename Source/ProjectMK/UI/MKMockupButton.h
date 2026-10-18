#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "MKMockupButton.generated.h"

// 목업 버튼을 누른 뒤에도 Space와 숫자 키가 게임 입력으로 전달되도록 한다.
UCLASS()
class PROJECTMK_API UMKMockupButton : public UButton
{
	GENERATED_BODY()

public:
	UMKMockupButton()
	{
		InitIsFocusable(false);
	}
};
