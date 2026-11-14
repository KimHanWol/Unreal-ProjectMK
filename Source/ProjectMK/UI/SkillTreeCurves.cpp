#include "ProjectMK/UI/SkillTreeCurves.h"

#include "ProjectMK/UI/SkillTreeLayout.h"

namespace
{
	double Cross(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }

	double PointDistanceSquared(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Delta = B - A;
		const double Ratio = FMath::Clamp(FVector2D::DotProduct(Point - A, Delta) / FMath::Max(0.0001, Delta.SizeSquared()), 0.0, 1.0);
		return FVector2D::DistSquared(Point, A + Delta * Ratio);
	}

	double SegmentDistanceSquared(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const double Denominator = Cross(B - A, D - C);
		if (FMath::Abs(Denominator) > 0.0001)
		{
			const double First = Cross(C - A, D - C) / Denominator;
			const double Second = Cross(C - A, B - A) / Denominator;
			if (First >= 0.0 && First <= 1.0 && Second >= 0.0 && Second <= 1.0) { return 0.0; }
		}
		return FMath::Min(FMath::Min(PointDistanceSquared(A, C, D), PointDistanceSquared(B, C, D)),
			FMath::Min(PointDistanceSquared(C, A, B), PointDistanceSquared(D, A, B)));
	}

	bool HitsBox(const FVector2D& A, const FVector2D& B, const FBox2D& Box)
	{
		double Lower = 0.0, Upper = 1.0;
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			const double Delta = B[Axis] - A[Axis];
			if (FMath::Abs(Delta) < 0.0001)
			{
				if (A[Axis] < Box.Min[Axis] || A[Axis] > Box.Max[Axis]) { return false; }
				continue;
			}
			const double First = (Box.Min[Axis] - A[Axis]) / Delta;
			const double Second = (Box.Max[Axis] - A[Axis]) / Delta;
			Lower = FMath::Max(Lower, FMath::Min(First, Second));
			Upper = FMath::Min(Upper, FMath::Max(First, Second));
			if (Lower > Upper) { return false; }
		}
		return true;
	}

	TArray<FVector2D> Simplify(const TArray<FVector2D>& Points)
	{
		TArray<FVector2D> Result;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			if (Index > 0 && Index + 1 < Points.Num()
				&& FVector2D::DotProduct((Points[Index] - Points[Index - 1]).GetSafeNormal(), (Points[Index + 1] - Points[Index]).GetSafeNormal()) > 0.9999) { continue; }
			Result.Add(Points[Index]);
		}
		return Result;
	}

	void AppendCubic(TArray<FVector2D>& Points, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const int32 Steps = FMath::Clamp(FMath::CeilToInt((FVector2D::Distance(A, B) + FVector2D::Distance(B, C) + FVector2D::Distance(C, D)) / 4.f), 8, 256);
		for (int32 Index = 1; Index <= Steps; ++Index)
		{
			const double T = static_cast<double>(Index) / Steps;
			const double Inverse = 1.0 - T;
			Points.Add(A * Inverse * Inverse * Inverse + B * 3.0 * Inverse * Inverse * T + C * 3.0 * Inverse * T * T + D * T * T * T);
		}
	}
}

void SkillTreeCurves::Apply(const TMap<FName, FVector2D>& NodeCenters, TArray<FSkillTreeLayoutEdge>& Edges)
{
	for (FSkillTreeLayoutEdge& Edge : Edges) { Edge.Points = Simplify(Edge.Points); }
	for (int32 EdgeIndex = 0; EdgeIndex < Edges.Num(); ++EdgeIndex)
	{
		FSkillTreeLayoutEdge& Edge = Edges[EdgeIndex];
		auto IsSafe = [&](const TArray<FVector2D>& Candidate)
		{
			for (int32 Segment = 1; Segment < Candidate.Num(); ++Segment)
			{
				const FVector2D A = Candidate[Segment - 1], B = Candidate[Segment];
				for (const auto& Node : NodeCenters)
				{
					const bool bOwner = Node.Key == Edge.Source || Node.Key == Edge.Target;
					const FVector2D Half = (Node.Key.IsNone() ? FVector2D(30.f, 38.f) : FVector2D(42.f)) + FVector2D(bOwner ? -0.25f : 10.f);
					if (HitsBox(A, B, FBox2D(Node.Value - Half, Node.Value + Half))) { return false; }
				}
				for (int32 Other = 0; Other < Edges.Num(); ++Other)
				{
					if (Other == EdgeIndex) { continue; }
					const TArray<FVector2D>& Path = Edges[Other].Points;
					for (int32 Index = 1; Index < Path.Num(); ++Index)
					{
						if (SegmentDistanceSquared(A, B, Path[Index - 1], Path[Index]) < FMath::Square(18.0)) { return false; }
					}
				}
			}
			return true;
		};
		const FVector2D Start = Edge.Points[0], End = Edge.Points.Last();
		const FVector2D FromDirection = (Edge.Points[1] - Start).GetSafeNormal();
		const FVector2D ToDirection = (End - Edge.Points[Edge.Points.Num() - 2]).GetSafeNormal();
		bool bFitted = false;
		for (float Ratio : { 0.35f, 0.2f, 0.5f })
		{
			const float Handle = FMath::Clamp(static_cast<float>(FVector2D::Distance(Start, End)) * Ratio, 28.f, 180.f);
			TArray<FVector2D> Curve = { Start };
			AppendCubic(Curve, Start, Start + FromDirection * Handle, End - ToDirection * Handle, End);
			if (IsSafe(Curve)) { Edge.Points = MoveTemp(Curve); bFitted = true; break; }
		}
		if (bFitted) { continue; }
		TArray<FVector2D> Shortcuts = { Start };
		int32 Current = 0;
		while (Current + 1 < Edge.Points.Num())
		{
			int32 Next = Current + 1;
			for (int32 Index = Edge.Points.Num() - 1; Index > Current + 1; --Index)
			{
				if (IsSafe({ Edge.Points[Current], Edge.Points[Index] })) { Next = Index; break; }
			}
			Shortcuts.Add(Edge.Points[Next]);
			Current = Next;
		}
		for (float Radius : { 96.f, 64.f, 40.f, 24.f, 12.f, 6.f })
		{
			TArray<FVector2D> Curve = { Start };
			for (int32 Index = 1; Index + 1 < Shortcuts.Num(); ++Index)
			{
				const FVector2D Before = Shortcuts[Index] - Shortcuts[Index - 1];
				const FVector2D After = Shortcuts[Index + 1] - Shortcuts[Index];
				const double Cut = FMath::Min(static_cast<double>(Radius), FMath::Min(Before.Size(), After.Size()) * 0.45);
				const FVector2D Entry = Shortcuts[Index] - Before.GetSafeNormal() * Cut;
				const FVector2D Exit = Shortcuts[Index] + After.GetSafeNormal() * Cut;
				Curve.Add(Entry);
				AppendCubic(Curve, Entry, Entry + (Shortcuts[Index] - Entry) * (2.0 / 3.0), Exit + (Shortcuts[Index] - Exit) * (2.0 / 3.0), Exit);
			}
			Curve.Add(End);
			if (IsSafe(Curve)) { Edge.Points = MoveTemp(Curve); bFitted = true; break; }
		}
		if (bFitted == false) { Edge.Points = MoveTemp(Shortcuts); }
	}
}
