#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MKGameInstance.generated.h"

class UDataManager;
class USoundManager;

UCLASS(Blueprintable)
class PROJECTMK_API UMKGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	virtual void Init() override;

public:
	UFUNCTION(BlueprintCallable)
	UDataManager* GetDataManager();

	UFUNCTION(BlueprintCallable)
	USoundManager* GetSoundManager();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDataManager> DataManagerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<USoundManager> SoundManagerClass;

private:
	void Apply2DRenderOverrides() const;

	UPROPERTY()
	UDataManager* DataManager;

	UPROPERTY()
	USoundManager* SoundManager;
};
