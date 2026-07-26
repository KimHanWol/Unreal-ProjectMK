#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/UI/MKUserWidget.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

#include "MKMenuContentsBase.generated.h"

UCLASS()
class PROJECTMK_API UMKMenuContentsBase : public UMKUserWidget
{
	GENERATED_BODY()

public:
	EMenuContentsType GetMenuContentsType() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	EMenuContentsType MenuContentsType = EMenuContentsType::None;
};
