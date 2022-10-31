// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.h"
#include "MetaSplineTemplateHelpers.h"
#include "MetaSpline.h"

#include "Algo/Find.h"
#include "BlueprintCompilationManager.h"

void UMetaSplineMetadata::InsertPoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0);

	if (NumCurves <= 0)
		return;

	Modify();

	const float InputKey = static_cast<float>(Index);

	if (Index >= NumPoints)
	{
		AddPoint(InputKey);
	}
	else
	{
		const int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
		const bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);

		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Points;
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

		NumPoints++;
	}
}

void UMetaSplineMetadata::UpdatePoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0 && Index < NumPoints);

	const int32 PrevIndex = (bClosedLoop && Index == 0 ? NumPoints - 1 : Index - 1);
	const int32 NextIndex = (bClosedLoop && Index + 1 > NumPoints ? 0 : Index + 1);

	const bool bHasPrevIndex = (PrevIndex >= 0 && PrevIndex < NumPoints);
	const bool bHasNextIndex = (NextIndex >= 0 && NextIndex < NumPoints);

	Modify();

	if (bHasPrevIndex && bHasNextIndex)
	{
		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Points;
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

	const int32 Index = NumPoints - 1;
	const float NewInputKey = static_cast<float>(Index + 1);

	TransformCurves([Index, NewInputKey](auto& Curve)
	{
		Curve.Points.Emplace(NewInputKey, Curve.Points[Index].OutVal);
	});

	NumPoints++;
}

void UMetaSplineMetadata::RemovePoint(int32 Index)
{
	check(Index < NumPoints);

	Modify();

	TransformCurves([Index](auto& Curve)
	{
		Curve.Points.RemoveAt(Index);
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
		Curve.Points.Insert({ Curve.Points[Index] }, Index);
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
			UE_LOG(LogMetaSpline, Error, TEXT("Can't copy point to spline with different meta class."));
			return;
		}

		Modify();

		TransformCurves([FromIndex, ToIndex, FromMetadata](FName Key, auto& Curve)
		{
			auto& Points = Curve.Points;

			using TUnderlyingType = TCurveUnderlyingType<decltype(Curve)>::Type;
			Points[ToIndex].OutVal = FromMetadata->FindCurve<TUnderlyingType>(Key)->Points[FromIndex].OutVal;
		});
	}
}

void UMetaSplineMetadata::Reset(int32 InNumPoints)
{
	Modify();
	NumPoints = InNumPoints;

	TransformCurves([this](auto& Curve)
	{
		Curve.Points.Reset(NumPoints);
	});
}

void UMetaSplineMetadata::Fixup(int32 InNumPoints, USplineComponent* SplineComp)
{
	const UMetaSplineComponent* MetaSpline = Cast<UMetaSplineComponent>(SplineComp);
	UpdateMetadataClass(MetaSpline ? MetaSpline->MetadataClass : nullptr);

	TransformPoints([](auto& Point, int32 Index)
	{
		Point.InVal = static_cast<float>(Index);
	});

	NumCurves = 0;
	TransformCurves([&](FName Key, auto& Curve)
	{
		NumCurves++;
		auto& Points = Curve.Points;

		if (!MetaClass)
			return;

		while (Points.Num() < InNumPoints)
		{
			const float InVal = Points.Num() > 0 ? Points[Points.Num() - 1].InVal + 1.0f : 0.0f;

			using TUnderlyingType = TCurveUnderlyingType<decltype(Curve)>::Type;
			using TCastToType = TCurveCastTo<TUnderlyingType>::Type;

			const FProperty* Property = MetaClass->FindPropertyByName(Key);
			Points.Add({ InVal, static_cast<TCastToType>(*Property->ContainerPtrToValuePtr<TUnderlyingType>(MetaClass->GetDefaultObject())) });
		}
	});

	TransformCurves([=](auto& Curve)
	{
		auto& Points = Curve.Points;
		if (Points.Num() > InNumPoints)
		{
			Points.RemoveAt(InNumPoints, Points.Num() - InNumPoints);
		}
	});

	NumPoints = InNumPoints;
}

template<typename T>
struct FAddCurve
{
	static void Execute(UMetaSplineMetadata& InOutMetadata, const FProperty* InProperty)
	{
		auto& Map = InOutMetadata.FindCurveMapForType<T>();
		if (Map.Contains(InProperty->GetFName()))
		{
			return;
		}

		auto& Curve = Map.Add(InProperty->GetFName(), {});

		const T& Value = *InProperty->ContainerPtrToValuePtr<T>(InOutMetadata.MetaClass->GetDefaultObject());
		for (int32 i = 0; i < InOutMetadata.NumPoints; i++)
		{
			Curve.AddPoint(i, Value);
		}

		InOutMetadata.NumCurves++;
	}
};

void UMetaSplineMetadata::UpdateMetadataClass(UClass* InClass)
{
	if (MetaClass && MetaClass == InClass)
	{
		// Three step process to update metadata with new or removed variables, but maintaining any values that 
		// should persist.

		// Step 1: Add all curves that don't exist already.
		for (FProperty* Property : TFieldRange<FProperty>(MetaClass))
		{
			FMetaSplineTemplateHelpers::ExecuteOnProperty<FAddCurve>(Property, *this, Property);
		}
		
		// Step 2: Remove curves that don't exist anymore.
		TransformCurveContainers([this](auto& Map)
		{
			TArray<FName> CurveNames;
			Map.GetKeys(CurveNames);
			for (const FName CurveName : CurveNames)
			{			
				if (!MetaClass->FindPropertyByName(CurveName))
				{
					Map.Remove(CurveName);
				}
			}
		});
		
		if (!CurrentDefaults)
			CurrentDefaults = MetaClass->GetDefaultObject();
		return;
	}

	TransformCurveContainers([](auto& Map) { Map.Empty(); });

	MetaClass = InClass;

	if (!MetaClass)
		return;

	CurrentDefaults = MetaClass->GetDefaultObject();

	for (FProperty* Property : TFieldRange<FProperty>(MetaClass))
	{
		FMetaSplineTemplateHelpers::ExecuteOnProperty<FAddCurve>(Property, *this, Property);
	}
}

#if WITH_EDITOR
void UMetaSplineMetadata::UpdateValuesOnUnmodifiedPoints()
{
	TransformCurves([this](FName CurveName, auto& Curve)
	{
		if (!CurrentDefaults)
		{
			return;
		}

		FProperty* OldProp = CurrentDefaults->GetClass()->FindPropertyByName(CurveName);
		FProperty* NewProp = MetaClass->FindPropertyByName(CurveName);
		if (!NewProp || !OldProp)
		{
			return;
		}

		using TUnderlyingType = TCurveUnderlyingType<decltype(Curve)>::Type;
		const auto& OldDefaultValue = *OldProp->ContainerPtrToValuePtr<TUnderlyingType>(CurrentDefaults);
		const auto& NewDefaultValue = *NewProp->ContainerPtrToValuePtr<TUnderlyingType>(MetaClass->GetDefaultObject());

		if (OldDefaultValue == NewDefaultValue)
		{
			return;
		}

		for (auto& Point : Curve.Points)
		{
			if (Point.OutVal == OldDefaultValue)
			{
				Point.OutVal = NewDefaultValue;
			}
		}
	});

	CurrentDefaults = MetaClass->GetDefaultObject();
}
#endif

void UMetaSplineMetadata::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	Super::PostTransacted(TransactionEvent);

	// Rerun construction script after each transaction.
	GetTypedOuter<AActor>()->PostEditMove(false);
}
