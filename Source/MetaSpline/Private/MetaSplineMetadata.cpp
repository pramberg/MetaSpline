// Fill out your copyright notice in the Description page of Project Settings.


#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.h"

void UMetaSplineMetadata::InsertPoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0);

	if (NumCurves <= 0)
		return;

	Modify();

	const float InputKey = static_cast<float>(Index);

	if (Index >= NumPoints)
	{
		// Just add point to the end instead of trying to insert
		AddPoint(InputKey);
	}
	else
	{
		const int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
		const bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);

		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Value.Points;
			auto NewValue = Points[Index].OutVal;

			if (bHasPrevIndex)
			{
				const auto& PrevVal = Points[PrevIndex].OutVal;
				NewValue = FMath::LerpStable(PrevVal, NewValue, t);
			}

			Points.Insert({ InputKey, NewValue }, Index);
		});

		TransformPoints(Index + 1, [](auto& Point)
		{
			Point.InVal += 1.0f;
		});
	}
}

void UMetaSplineMetadata::UpdatePoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0 && Index < NumPoints);

	int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
	int32 NextIndex = (bClosedLoop && Index + 1 > NumPoints ? 0 : Index + 1);

	bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);
	bool bHasNextIndex = (NextIndex >= 0 && NextIndex < NumPoints);

	Modify();

	if (bHasPrevIndex && bHasNextIndex)
	{
		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Value.Points;
			const auto& PrevVal = Points[PrevIndex].OutVal;
			const auto& NextVal = Points[NextIndex].OutVal;

			Points[Index].OutVal = FMath::LerpStable(PrevVal, NextVal, t);
		});
	}
}

void UMetaSplineMetadata::AddPoint(float InputKey)
{
	if (NumCurves <= 0)
		return;

	Modify();

	const int Index = NumPoints - 1;
	float NewInputKey = static_cast<float>(Index + 1);

	TransformCurves([Index, NewInputKey](auto& Curve)
	{
		Curve.Value.Points.Emplace(NewInputKey, Curve.Value.Points[Index].OutVal);
	});

	NumPoints++;
}

void UMetaSplineMetadata::RemovePoint(int32 Index)
{
	check(Index < NumPoints);

	Modify();

	TransformCurves([Index](auto& Curve)
	{
		Curve.Value.Points.RemoveAt(Index);
	});

	TransformPoints(Index, [](auto& Point)
	{
		Point.InVal -= 1.0f;
	});

	NumPoints--;
}

void UMetaSplineMetadata::DuplicatePoint(int32 Index)
{
	check(Index < NumPoints);

	Modify();

	TransformCurves([Index](auto& Curve)
	{
		Curve.Value.Points.Insert({ Curve.Value.Points[Index] }, Index);
	});

	TransformPoints(Index + 1, [](auto& Point)
	{
		Point.InVal += 1.0f;
	});

	NumPoints++;
}

void UMetaSplineMetadata::CopyPoint(const USplineMetadata* FromSplineMetadata, int32 FromIndex, int32 ToIndex)
{
	check(FromSplineMetadata != nullptr);

	if (const UMetaSplineMetadata* FromMetadata = Cast<UMetaSplineMetadata>(FromSplineMetadata))
	{
		check(ToIndex < NumPoints);
		check(FromIndex < FromMetadata->NumPoints);

		if (FromMetadata->MetaClass != MetaClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Can't copy point to spline with different meta class."));
			return;
		}

		Modify();

		TransformCurves([FromIndex, ToIndex, FromMetadata](auto& Curve)
		{
			auto& Points = Curve.Value.Points;
			
			using TCurveType = typename TDecay<decltype(Curve.Value)>::Type;
			Points[ToIndex].OutVal = FromMetadata->FindCurve<TCurveType>(Curve.Key)->Points[FromIndex].OutVal;
		});
	}
}

void UMetaSplineMetadata::Reset(int32 InNumPoints)
{
	Modify();
	NumPoints = InNumPoints;

	TransformCurves([this](auto& Curve)
	{
		Curve.Value.Points.Reset(NumPoints);
	});
}

void UMetaSplineMetadata::Fixup(int32 InNumPoints, USplineComponent* SplineComp)
{
	const UMetaSplineComponent* MetaSpline = Cast<UMetaSplineComponent>(SplineComp);
	MetaClass = MetaSpline ? MetaSpline->MetadataClass : nullptr;

	TransformPointsIndex([](auto& Point, int32 Index)
	{
		Point.InVal = static_cast<float>(Index);
	});

	TransformCurves([&](auto& Curve)
	{
		auto& Points = Curve.Value.Points;

		if (!MetaClass)
			return;

		while (Points.Num() < InNumPoints)
		{
			const float InVal = Points.Num() > 0 ? Points[Points.Num() - 1].InVal + 1.0f : 0.0f;

			using TPointsArray = typename TDecay<decltype(Points)>::Type;
			using TPoint = typename TPointsArray::ElementType;
			using TUnderlyingType = typename TCurvePointUnderlyingType<TPoint>::Type;

			const FProperty* Property = MetaClass->FindPropertyByName(Curve.Key);
			Points.Add({ InVal, *Property->ContainerPtrToValuePtr<TUnderlyingType>(MetaClass->GetDefaultObject()) });
		}
	});

	TransformCurves([=](auto& Curve)
	{
		auto& Points = Curve.Value.Points;
		if (Points.Num() > InNumPoints)
		{
			Points.RemoveAt(InNumPoints, Points.Num() - InNumPoints);
		}
	});

	NumPoints = InNumPoints;
}

#pragma optimize("", off)
void UMetaSplineMetadata::UpdateMetadataClass(UClass* InClass)
{
	FloatCurves.Empty();
	VectorCurves.Empty();

	/*TArray<FName> CurvesToRemove;
	FloatCurves.GetKeys(CurvesToRemove);
	VectorCurves.GetKeys(CurvesToRemove);*/

	MetaClass = InClass;

	if (!MetaClass)
		return;

	for (TFieldIterator<FProperty> It(MetaClass); It; ++It)
	{
		FProperty* Property = *It;

		const FName Type = FName(Property->GetCPPType());
		if (Type == TEXT("float"))
			AddCurve<float>(FloatCurves, Property);
		else if (Type == TEXT("FVector"))
			AddCurve<FVector>(VectorCurves, Property);
		/*else if (Type == TEXT("FVector2D"))
			Curves.Add(Property->GetFName(), FInterpCurveDesc{ EInterpCurveType::Vector2D });
		else if (Type == TEXT("FLinearColor"))
			Curves.Add(Property->GetFName(), FInterpCurveDesc{ EInterpCurveType::LinearColor });
		else if (Type == TEXT("FQuat"))
			Curves.Add(Property->GetFName(), FInterpCurveDesc{ EInterpCurveType::Quat });
		else if (Type == TEXT("FRotator"))
			Curves.Add(Property->GetFName(), FInterpCurveDesc{ EInterpCurveType::Quat });
		else if (Property->GetOwnerClass())
			Curves.Add(Property->GetFName(), FInterpCurveDesc{ EInterpCurveType::Object });*/
	}
	
}
#pragma optimize("", on)