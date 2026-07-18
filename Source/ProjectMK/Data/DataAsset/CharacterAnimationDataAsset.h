#pragma once

#include "CoreMinimal.h"
#include "PaperSprite.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "ProjectMK/Data/Struct/CharacterAnimationTextureSet.h"

#include "CharacterAnimationDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FCharacterSpriteAnimationClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TArray<TObjectPtr<UPaperSprite>> Sprites;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (ClampMin = "0.0"))
	float PlayRate = 1.f;

	const UPaperSprite* GetSpriteByFrameIndex(int32 FrameIndex) const
	{
		if (Sprites.IsEmpty())
		{
			return nullptr;
		}

		const int32 SafeFrameIndex = FMath::Clamp(FrameIndex, 0, Sprites.Num() - 1);
		return Sprites[SafeFrameIndex];
	}

	bool HasSprites() const
	{
		return Sprites.IsEmpty() == false;
	}
};

USTRUCT(BlueprintType)
struct FCharacterSpriteAnimationSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip IdleDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip IdleUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip IdleLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip IdleRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip WalkLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationClip WalkRight;

	const FCharacterSpriteAnimationClip* FindClip(ECharacterAnimationType AnimationType) const
	{
		switch (AnimationType)
		{
		case ECharacterAnimationType::Idle_Down:
			return &IdleDown;
		case ECharacterAnimationType::Idle_Up:
			return &IdleUp;
		case ECharacterAnimationType::Idle_Left:
			return &IdleLeft;
		case ECharacterAnimationType::Idle_Right:
			return &IdleRight;
		case ECharacterAnimationType::Walk_Left:
			return &WalkLeft;
		case ECharacterAnimationType::Walk_Right:
			return &WalkRight;
		default:
			return nullptr;
		}
	}
};

UCLASS(BlueprintType)
class PROJECTMK_API UCharacterAnimationDataAsset : public UMKDataAssetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FCharacterSpriteAnimationSet Animations;
};
