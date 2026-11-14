#include "ProjectMK/UI/SkillTreeLayout.h"

#include "Algo/Reverse.h"
#include "ProjectMK/UI/SkillTreeCurves.h"

#include <algorithm>
#include <queue>

namespace
{
	constexpr float CellSize = 21.f;
	constexpr float NodeHalfSize = 42.f;
	constexpr int32 InitialLayoutSeed = 1863;

	struct FPort
	{
		FIntPoint Cell;
		FIntPoint Direction;
	};

	struct FQueueCell
	{
		FIntPoint Cell;
		int32 Cost;
		int32 Score;
		bool operator<(const FQueueCell& Other) const
		{
			if (Score != Other.Score) { return Score > Other.Score; }
			if (Cost != Other.Cost) { return Cost > Other.Cost; }
			if (Cell.X != Other.Cell.X) { return Cell.X > Other.Cell.X; }
			return Cell.Y > Other.Cell.Y;
		}
	};

	int32 Distance(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

	TArray<FPort> GetPorts(FName Key, const FIntPoint& Center)
	{
		const FIntPoint Half(2, 2);
		TArray<FPort> Ports;
		for (const FIntPoint Direction : { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) })
		{
			for (int32 Offset = -1; Offset <= 1; ++Offset)
			{
				Ports.Add({ Center + FIntPoint((Half.X + 1) * Direction.X + Offset * Direction.Y,
					(Half.Y + 1) * Direction.Y + Offset * Direction.X), Direction });
			}
		}
		return Ports;
	}

	bool FindPath(const FIntPoint& Start, const FIntPoint& Goal, const FIntRect& Bounds,
		const TSet<FIntPoint>& Blocked, const TSet<FIntPoint>& Occupied, TArray<FIntPoint>& OutCells)
	{
		std::priority_queue<FQueueCell> Open;
		Open.push({ Start, 0, Distance(Start, Goal) * 10 });
		TMap<FIntPoint, FIntPoint> Parent;
		TMap<FIntPoint, int32> Costs;
		TSet<FIntPoint> Closed;
		Costs.Add(Start, 0);
		while (Open.empty() == false)
		{
			const FQueueCell Current = Open.top();
			Open.pop();
			if (Closed.Contains(Current.Cell)) { continue; }
			if (Current.Cell == Goal)
			{
				OutCells = { Goal };
				while (OutCells.Last() != Start) { OutCells.Add(Parent.FindChecked(OutCells.Last())); }
				Algo::Reverse(OutCells);
				return true;
			}
			Closed.Add(Current.Cell);
			for (const FIntPoint Direction : { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) })
			{
				const FIntPoint Next = Current.Cell + Direction;
				if (Bounds.Contains(Next) == false || Blocked.Contains(Next) || Occupied.Contains(Next) || Closed.Contains(Next)) { continue; }
				const FIntPoint* Previous = Parent.Find(Current.Cell);
				const int32 Cost = Current.Cost + 10 + (Previous != nullptr && Current.Cell - *Previous != Direction ? 4 : 0);
				const int32* Existing = Costs.Find(Next);
				if (Existing == nullptr || Cost < *Existing)
				{
					Costs.Add(Next, Cost);
					Parent.Add(Next, Current.Cell);
					Open.push({ Next, Cost, Cost + Distance(Next, Goal) * 10 });
				}
			}
		}
		return false;
	}

	bool RouteEdges(const TMap<FName, FIntPoint>& Positions, TArray<FSkillTreeLayoutEdge>& Edges)
	{
		TSet<FIntPoint> Blocked, Occupied;
		FIntRect Search(MAX_int32, MAX_int32, MIN_int32, MIN_int32);
		for (const auto& Pair : Positions)
		{
			const FIntPoint Half(2, 2);
			Search.Min.X = FMath::Min(Search.Min.X, Pair.Value.X - 12);
			Search.Min.Y = FMath::Min(Search.Min.Y, Pair.Value.Y - 12);
			Search.Max.X = FMath::Max(Search.Max.X, Pair.Value.X + 13);
			Search.Max.Y = FMath::Max(Search.Max.Y, Pair.Value.Y + 13);
			for (int32 Y = -Half.Y; Y <= Half.Y; ++Y)
			{
				for (int32 X = -Half.X; X <= Half.X; ++X) { Blocked.Add(Pair.Value + FIntPoint(X, Y)); }
			}
		}
		Edges.StableSort([&Positions](const FSkillTreeLayoutEdge& A, const FSkillTreeLayoutEdge& B)
		{
			return (Positions[A.Source] - Positions[A.Target]).SizeSquared() > (Positions[B.Source] - Positions[B.Target]).SizeSquared();
		});
		for (FSkillTreeLayoutEdge& Edge : Edges)
		{
			struct FPortPair { FPort From; FPort To; int32 Cost; };
			TArray<FPortPair> Options;
			for (const FPort& From : GetPorts(Edge.Source, Positions[Edge.Source]))
			{
				for (const FPort& To : GetPorts(Edge.Target, Positions[Edge.Target]))
				{
					if (Occupied.Contains(From.Cell) == false && Occupied.Contains(To.Cell) == false) { Options.Add({ From, To, Distance(From.Cell, To.Cell) }); }
				}
			}
			Options.StableSort([](const FPortPair& A, const FPortPair& B) { return A.Cost < B.Cost; });
			bool bRouted = false;
			for (int32 Index = 0; Index < FMath::Min(30, Options.Num()); ++Index)
			{
				const FPortPair& Option = Options[Index];
				TArray<FIntPoint> Cells;
				if (FindPath(Option.From.Cell, Option.To.Cell, Search, Blocked, Occupied, Cells) == false
					|| Cells.Num() > Distance(Positions[Edge.Source], Positions[Edge.Target]) + 20) { continue; }
				// Preserve the transverse port offset and trim only along its outward normal.
				auto EndPoint = [&Positions](FName Key, const FPort& Port)
				{
					const FVector2D Half = Key.IsNone() ? FVector2D(30.f, 38.f) : FVector2D(NodeHalfSize);
					FVector2D Point = FVector2D(Port.Cell) * CellSize;
					if (Port.Direction.X != 0) { Point.X = Positions[Key].X * CellSize + Port.Direction.X * Half.X; }
					else { Point.Y = Positions[Key].Y * CellSize + Port.Direction.Y * Half.Y; }
					return Point;
				};
				Edge.Points = { EndPoint(Edge.Source, Option.From) };
				for (const FIntPoint Cell : Cells) { Occupied.Add(Cell); Edge.Points.Add(FVector2D(Cell) * CellSize); }
				Edge.Points.Add(EndPoint(Edge.Target, Option.To));
				bRouted = true;
				break;
			}
			if (bRouted == false) { return false; }
		}
		return true;
	}
}

bool FSkillTreeLayout::Calculate(const TMap<FName, TArray<FName>>& Prerequisites, FSkillTreeLayout& OutLayout)
{
	// Widget reconstruction can ask for the same pure layout several times during PIE startup.
	static TMap<FName, TArray<FName>> CachedPrerequisites;
	static FSkillTreeLayout CachedLayout;
	static bool bHasCachedLayout = false;
	if (bHasCachedLayout && CachedPrerequisites.OrderIndependentCompareEqual(Prerequisites))
	{
		OutLayout = CachedLayout;
		return true;
	}
	TArray<FName> Keys;
	Prerequisites.GetKeys(Keys);
	Keys.Sort(FNameLexicalLess());
	TMap<FName, TArray<FName>> Children;
	Children.Add(NAME_None);
	for (FName Key : Keys) { Children.Add(Key); }
	TArray<FSkillTreeLayoutEdge> Edges;
	TMap<FName, FName> PrimaryParents;
	for (FName Key : Keys)
	{
		FName Parent;
		for (FName Required : Prerequisites[Key])
		{
			if (Prerequisites.Contains(Required))
			{
				if (Parent.IsNone()) { Parent = Required; }
				Edges.Add({ Required, Key, {} });
			}
		}
		Children[Parent].Add(Key);
		PrimaryParents.Add(Key, Parent);
		if (Parent.IsNone()) { Edges.Add({ NAME_None, Key, {} }); }
	}
	TMap<FName, int32> Weights;
	TSet<FName> Visiting;
	TFunction<int32(FName)> Weight = [&](FName Key)
	{
		if (Visiting.Contains(Key)) { return 0; }
		Visiting.Add(Key);
		int32 Total = 0;
		for (FName Child : Children[Key]) { Total += Weight(Child); }
		const int32 Value = FMath::Max(1, Total);
		Weights.Add(Key, Value);
		return Value;
	};
	Weight(NAME_None);
	if (Weights.Num() != Keys.Num() + 1) { return false; }
	TMap<FName, FName> Families;
	for (FName Key : Keys)
	{
		FName Root = Key;
		while (PrimaryParents[Root].IsNone() == false) { Root = PrimaryParents[Root]; }
		Families.Add(Key, Root);
	}
	TArray<FName> Roots = Children[NAME_None];
	Roots.Sort(FNameLexicalLess());
	TArray<FName> RootOrder = Roots;
	if (Roots.Num() > 1 && Roots.Num() <= 8)
	{
		TArray<FName> Permutation = Roots;
		Permutation.RemoveAt(0);
		Algo::Reverse(Permutation);
		int32 BestCost = MAX_int32;
		do
		{
			TArray<FName> Candidate = { Roots[0] };
			Candidate.Append(Permutation);
			int32 Cost = 0;
			for (const FSkillTreeLayoutEdge& Edge : Edges)
			{
				if (Edge.Source.IsNone() || Families[Edge.Source] == Families[Edge.Target]) { continue; }
				const int32 Difference = FMath::Abs(Candidate.IndexOfByKey(Families[Edge.Source]) - Candidate.IndexOfByKey(Families[Edge.Target]));
				Cost += FMath::Min(Difference, Candidate.Num() - Difference);
			}
			if (Cost < BestCost) { BestCost = Cost; RootOrder = Candidate; }
		}
		while (std::prev_permutation(Permutation.GetData(), Permutation.GetData() + Permutation.Num(), FNameLexicalLess()));
	}
	TMap<FName, int32> RootWidths;
	int32 TotalRootWidth = 0;
	for (FName Root : Roots) { const int32 Width = FMath::Max(3, Weights[Root]); RootWidths.Add(Root, Width); TotalRootWidth += Width; }
	const float StartAngle = Roots.IsEmpty() ? -PI : -PI * RootWidths[RootOrder[0]] / TotalRootWidth;
	TMap<FName, float> RootAngles;
	float Cursor = StartAngle;
	for (FName Root : RootOrder)
	{
		const float Width = 2.f * PI * RootWidths[Root] / TotalRootWidth;
		RootAngles.Add(Root, Cursor + Width * 0.5f);
		Cursor += Width;
	}
	TMap<FName, TSet<FName>> Descendants;
	TFunction<void(FName, TSet<FName>&)> Collect = [&](FName Key, TSet<FName>& Result)
	{
		Result.Add(Key);
		for (FName Child : Children[Key]) { Collect(Child, Result); }
	};
	for (FName Key : Keys) { Collect(Key, Descendants.Add(Key)); }
	TMap<FName, float> Pulls;
	for (FName Key : Keys)
	{
		float Sum = 0.f;
		int32 Count = 0;
		for (const FSkillTreeLayoutEdge& Edge : Edges)
		{
			if (Edge.Source.IsNone() || Families[Edge.Source] == Families[Edge.Target]) { continue; }
			FName Other;
			if (Descendants[Key].Contains(Edge.Source)) { Other = Edge.Target; }
			else if (Descendants[Key].Contains(Edge.Target)) { Other = Edge.Source; }
			if (Other.IsNone() == false)
			{
				Sum += FMath::FindDeltaAngleRadians(RootAngles[Families[Key]], RootAngles[Families[Other]]);
				++Count;
			}
		}
		Pulls.Add(Key, Count > 0 ? Sum / Count : 0.f);
	}
	for (int32 Attempt = 0; Attempt < 2048; ++Attempt)
	{
		FRandomStream Random(Attempt + InitialLayoutSeed);
		auto Order = Children;
		TArray<FName> OrderedKeys = Keys;
		OrderedKeys.Insert(NAME_None, 0);
		for (FName Key : OrderedKeys)
		{
			TArray<FName>& Branch = Order[Key];
			if (Key.IsNone()) { Branch = RootOrder; continue; }
			if (Attempt == 0) { Branch.StableSort([&Pulls](FName A, FName B) { return Pulls[A] < Pulls[B]; }); }
			else { for (int32 Index = Branch.Num() - 1; Index > 0; --Index) { Branch.Swap(Index, Random.RandRange(0, Index)); } }
		}
		TMap<FName, FIntPoint> Positions;
		Positions.Add(NAME_None, FIntPoint::ZeroValue);
		TFunction<void(FName, float, float, int32)> Place = [&](FName Key, float Lower, float Upper, int32 Depth)
		{
			if (Key.IsNone() == false)
			{
				const float Angle = (Lower + Upper) * 0.5f;
				Positions.Add(Key, FIntPoint(FMath::RoundToInt(Depth * 10.f * FMath::Cos(Angle)), FMath::RoundToInt(Depth * 10.f * FMath::Sin(Angle))));
			}
			float Cursor = Lower;
			for (FName Child : Order[Key])
			{
				const float Width = (Upper - Lower) * (Key.IsNone() ? static_cast<float>(RootWidths[Child]) / TotalRootWidth : static_cast<float>(Weights[Child]) / Weights[Key]);
				Place(Child, Cursor, Cursor + Width, Depth + 1);
				Cursor += Width;
			}
		};
		Place(NAME_None, StartAngle, StartAngle + 2.f * PI, 0);
		bool bSeparated = true;
		for (int32 A = 0; A < OrderedKeys.Num() && bSeparated; ++A)
		{
			for (int32 B = A + 1; B < OrderedKeys.Num(); ++B)
			{
				const FIntPoint Delta = Positions[OrderedKeys[A]] - Positions[OrderedKeys[B]];
				if (FMath::Abs(Delta.X) < 6 && FMath::Abs(Delta.Y) < 6) { bSeparated = false; break; }
			}
		}
		if (bSeparated == false) { continue; }
		TArray<FSkillTreeLayoutEdge> Routed = Edges;
		if (RouteEdges(Positions, Routed) == false) { continue; }
		TMap<FName, FVector2D> NodeCenters;
		for (const auto& Pair : Positions) { NodeCenters.Add(Pair.Key, FVector2D(Pair.Value) * CellSize); }
		SkillTreeCurves::Apply(NodeCenters, Routed);
		OutLayout = FSkillTreeLayout();
		for (const auto& Pair : Positions)
		{
			const FVector2D Point = FVector2D(Pair.Value) * CellSize;
			OutLayout.Positions.Add(Pair.Key, Point - FVector2D(NodeHalfSize));
			const FVector2D Half = Pair.Key.IsNone() ? FVector2D(30.f, 38.f) : FVector2D(NodeHalfSize);
			OutLayout.Bounds += Point - Half;
			OutLayout.Bounds += Point + Half;
		}
		OutLayout.Edges = MoveTemp(Routed);
		for (const FSkillTreeLayoutEdge& Edge : OutLayout.Edges) { for (const FVector2D& Point : Edge.Points) { OutLayout.Bounds += Point; } }
		CachedPrerequisites = Prerequisites;
		CachedLayout = OutLayout;
		bHasCachedLayout = true;
		return true;
	}
	return false;
}
