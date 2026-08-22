#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

#include "MagnetRangeComponent.generated.h"

class FMagnetRangeSceneProxy;

UCLASS(ClassGroup = (ProjectMK), meta = (BlueprintSpawnableComponent))
class PROJECTMK_API UMagnetRangeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UMagnetRangeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	void StartExpansion(float TargetRadius);
	void StartCollapse();
	void SetExpandedRadius(float NewRadius);

	DECLARE_MULTICAST_DELEGATE(FOnExpansionFinished);
	FOnExpansionFinished OnExpansionFinishedDelegate;

private:
	friend class FMagnetRangeSceneProxy;

	enum class EAnimationState : uint8
	{
		Hidden,
		Expanding,
		Expanded,
		Collapsing
	};

	void StartRadiusAnimation(float TargetRadius);
	void SetRadius(float NewRadius);

	UPROPERTY(Transient)
	float Radius = 0.f;

	UPROPERTY(Transient)
	float ExpandedRadius = 0.f;

	UPROPERTY(Transient)
	float AnimationStartRadius = 0.f;

	UPROPERTY(Transient)
	float AnimationTargetRadius = 0.f;

	UPROPERTY(Transient)
	float AnimationElapsedTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Magnet Range", meta = (ClampMin = "0.01"))
	float AnimationDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Magnet Range")
	FLinearColor CircleColor = FLinearColor::Yellow;

	UPROPERTY(EditDefaultsOnly, Category = "Magnet Range", meta = (ClampMin = "0.1"))
	float LineThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Magnet Range", meta = (ClampMin = "12"))
	int32 SegmentCount = 64;

	EAnimationState AnimationState = EAnimationState::Hidden;
};
