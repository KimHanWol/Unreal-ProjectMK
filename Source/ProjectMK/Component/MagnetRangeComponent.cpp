// LINK

#include "ProjectMK/Component/MagnetRangeComponent.h"

#include "PrimitiveDrawingUtils.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"

class FMagnetRangeSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FMagnetRangeSceneProxy(const UMagnetRangeComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, Radius(Component->Radius)
		, CircleColor(Component->CircleColor)
		, LineThickness(Component->LineThickness)
		, SegmentCount(Component->SegmentCount)
	{
	}

	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			if ((VisibilityMap & (1u << ViewIndex)) == 0)
			{
				continue;
			}

			FPrimitiveDrawInterface* PrimitiveDrawInterface = Collector.GetPDI(ViewIndex);
			const FMatrix& ComponentToWorld = GetLocalToWorld();
			const FVector CircleCenter = ComponentToWorld.GetOrigin();
			const FVector CircleXAxis = ComponentToWorld.TransformVector(FVector::ForwardVector).GetSafeNormal();
			const FVector CircleZAxis = ComponentToWorld.TransformVector(FVector::UpVector).GetSafeNormal();
			DrawCircle(
				PrimitiveDrawInterface,
				CircleCenter,
				CircleXAxis,
				CircleZAxis,
				CircleColor,
				Radius,
				SegmentCount,
				SDPG_Foreground,
				LineThickness,
				0.f,
				true);
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance ViewRelevance;
		ViewRelevance.bDrawRelevance = IsShown(View);
		ViewRelevance.bDynamicRelevance = true;
		ViewRelevance.bRenderInMainPass = ShouldRenderInMainPass();
		ViewRelevance.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return ViewRelevance;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	uint32 GetAllocatedSize() const
	{
		return FPrimitiveSceneProxy::GetAllocatedSize();
	}

private:
	float Radius = 0.f;
	FLinearColor CircleColor = FLinearColor::Yellow;
	float LineThickness = 2.f;
	int32 SegmentCount = 64;
};

UMagnetRangeComponent::UMagnetRangeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
}

void UMagnetRangeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AnimationElapsedTime += DeltaTime;
	const float AnimationAlpha = FMath::Clamp(AnimationElapsedTime / FMath::Max(AnimationDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const float EasedAnimationAlpha = FMath::InterpEaseInOut(0.f, 1.f, AnimationAlpha, 2.f);
	SetRadius(FMath::Lerp(AnimationStartRadius, AnimationTargetRadius, EasedAnimationAlpha));

	if (AnimationAlpha < 1.f)
	{
		return;
	}

	SetComponentTickEnabled(false);
	if (AnimationState == EAnimationState::Expanding)
	{
		AnimationState = EAnimationState::Expanded;
		OnExpansionFinishedDelegate.Broadcast();
		return;
	}

	AnimationState = EAnimationState::Hidden;
	SetVisibility(false);
}

FPrimitiveSceneProxy* UMagnetRangeComponent::CreateSceneProxy()
{
	if (Radius <= 0.f)
	{
		return nullptr;
	}

	return new FMagnetRangeSceneProxy(this);
}

FBoxSphereBounds UMagnetRangeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FVector::ZeroVector, FVector(Radius, 1.f, Radius), Radius).TransformBy(LocalToWorld);
}

void UMagnetRangeComponent::StartExpansion(float TargetRadius)
{
	ExpandedRadius = FMath::Max(0.f, TargetRadius);
	if (ExpandedRadius <= 0.f)
	{
		StartCollapse();
		return;
	}

	SetVisibility(true);
	AnimationState = EAnimationState::Expanding;
	StartRadiusAnimation(ExpandedRadius);
}

void UMagnetRangeComponent::StartCollapse()
{
	if (AnimationState == EAnimationState::Hidden)
	{
		SetRadius(0.f);
		SetVisibility(false);
		return;
	}

	AnimationState = EAnimationState::Collapsing;
	StartRadiusAnimation(0.f);
}

void UMagnetRangeComponent::SetExpandedRadius(float NewRadius)
{
	ExpandedRadius = FMath::Max(0.f, NewRadius);

	switch (AnimationState)
	{
	case EAnimationState::Expanding:
		if (ExpandedRadius > 0.f)
		{
			StartRadiusAnimation(ExpandedRadius);
		}
		else
		{
			StartCollapse();
		}
		break;
	case EAnimationState::Expanded:
		if (ExpandedRadius > 0.f)
		{
			SetRadius(ExpandedRadius);
		}
		else
		{
			StartCollapse();
		}
		break;
	default:
		break;
	}
}

void UMagnetRangeComponent::StartRadiusAnimation(float TargetRadius)
{
	AnimationStartRadius = Radius;
	AnimationTargetRadius = FMath::Max(0.f, TargetRadius);
	AnimationElapsedTime = 0.f;

	if (FMath::IsNearlyEqual(AnimationStartRadius, AnimationTargetRadius))
	{
		SetRadius(AnimationTargetRadius);
		SetComponentTickEnabled(false);

		if (AnimationState == EAnimationState::Expanding)
		{
			AnimationState = EAnimationState::Expanded;
			OnExpansionFinishedDelegate.Broadcast();
		}
		else
		{
			AnimationState = EAnimationState::Hidden;
			SetVisibility(false);
		}
		return;
	}

	SetComponentTickEnabled(true);
}

void UMagnetRangeComponent::SetRadius(float NewRadius)
{
	const float ClampedRadius = FMath::Max(0.f, NewRadius);
	if (FMath::IsNearlyEqual(Radius, ClampedRadius))
	{
		return;
	}

	Radius = ClampedRadius;
	UpdateBounds();
	MarkRenderStateDirty();
}
