#pragma once

#include "CoreMinimal.h"

#include "CharacterAnimationTextureSet.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ECharacterAnimationType : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Run UMETA(DisplayName = "Run"),
	Fly UMETA(DisplayName = "Fly"),
	Fall UMETA(DisplayName = "Fall"),
	Drill_Side UMETA(DisplayName = "Drill_Side"),
	Drill_Up UMETA(DisplayName = "Drill_Up"),
	Drill_Down UMETA(DisplayName = "Drill_Down"),
};

USTRUCT(BlueprintType)
struct FCharacterAnimationTextureSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UTexture2D> IdleTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UTexture2D> RunTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UTexture2D> FlyTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UTexture2D> FallTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (DisplayName = "Drill Side Texture"))
	TSoftObjectPtr<UTexture2D> DrillSideTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (DisplayName = "Drill Up Texture"))
	TSoftObjectPtr<UTexture2D> DrillUpTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (DisplayName = "Drill Down Texture"))
	TSoftObjectPtr<UTexture2D> DrillDownTexture;

	TSoftObjectPtr<UTexture2D>* FindTextureMutable(ECharacterAnimationType AnimationType)
	{
		switch (AnimationType)
		{
		case ECharacterAnimationType::Idle:
			return &IdleTexture;
		case ECharacterAnimationType::Run:
			return &RunTexture;
		case ECharacterAnimationType::Fly:
			return &FlyTexture;
		case ECharacterAnimationType::Fall:
			return &FallTexture;
		case ECharacterAnimationType::Drill_Side:
			return &DrillSideTexture;
		case ECharacterAnimationType::Drill_Up:
			return &DrillUpTexture;
		case ECharacterAnimationType::Drill_Down:
			return &DrillDownTexture;
		default:
			return nullptr;
		}
	}

	const TSoftObjectPtr<UTexture2D>* FindTexture(ECharacterAnimationType AnimationType) const
	{
		switch (AnimationType)
		{
		case ECharacterAnimationType::Idle:
			return &IdleTexture;
		case ECharacterAnimationType::Run:
			return &RunTexture;
		case ECharacterAnimationType::Fly:
			return &FlyTexture;
		case ECharacterAnimationType::Fall:
			return &FallTexture;
		case ECharacterAnimationType::Drill_Side:
			return &DrillSideTexture;
		case ECharacterAnimationType::Drill_Up:
			return &DrillUpTexture;
		case ECharacterAnimationType::Drill_Down:
			return &DrillDownTexture;
		default:
			return nullptr;
		}
	}

	void SetTexture(ECharacterAnimationType AnimationType, const TSoftObjectPtr<UTexture2D>& Texture)
	{
		TSoftObjectPtr<UTexture2D>* TargetTexture = FindTextureMutable(AnimationType);
		if (TargetTexture != nullptr)
		{
			*TargetTexture = Texture;
		}
	}

	bool HasAnyTexture() const
	{
		return IdleTexture.IsNull() == false
			|| RunTexture.IsNull() == false
			|| FlyTexture.IsNull() == false
			|| FallTexture.IsNull() == false
			|| DrillSideTexture.IsNull() == false
			|| DrillUpTexture.IsNull() == false
			|| DrillDownTexture.IsNull() == false;
	}
};
