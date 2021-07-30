// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include <UObject/UnrealType.h>
#include <utility>
#include "MetaSplineMetadata.generated.h"

template<typename T> struct TCurveUnderlyingType { using Type = void; };
template<typename T> struct TCurveUnderlyingType<FInterpCurve<T>> { using Type = T; };
template<> struct TCurveUnderlyingType<FInterpCurveFloat> { using Type = float; };
template<> struct TCurveUnderlyingType<FInterpCurveVector> { using Type = FVector; };
template<> struct TCurveUnderlyingType<FInterpCurveQuat> { using Type = FQuat; };
template<> struct TCurveUnderlyingType<FInterpCurveLinearColor> { using Type = FLinearColor; };
template<> struct TCurveUnderlyingType<FInterpCurveVector2D> { using Type = FVector2D; };

/**
 * Holds the actual curves that are generated from the meta class.
 */
UCLASS()
class UMetaSplineMetadata : public USplineMetadata
{
	GENERATED_BODY()

public:
	virtual void InsertPoint(int32 Index, float t, bool bClosedLoop) override;
	virtual void UpdatePoint(int32 Index, float t, bool bClosedLoop) override;
	virtual void AddPoint(float InputKey) override;
	virtual void RemovePoint(int32 Index) override;
	virtual void DuplicatePoint(int32 Index) override;
	virtual void CopyPoint(const USplineMetadata* FromSplineMetadata, int32 FromIndex, int32 ToIndex) override;
	virtual void Reset(int32 NumPoints) override;
	virtual void Fixup(int32 NumPoints, USplineComponent* SplineComp) override;

public:
	void UpdateMetadataClass(UClass* InClass);
	bool HasValidMetadataClass() const { return MetaClass ? true : false; }

	template<typename T>
	const FInterpCurve<T>* FindCurve(const FName InName) const
	{
		if constexpr (TIsSame<T, float>::Value)
			return FloatCurves.Find(InName);
		else if constexpr (TIsSame<T, FVector>::Value)
			return VectorCurves.Find(InName);
		return nullptr;
	}

	template<typename T>
	FInterpCurve<T>* FindCurve(const FName InName)
	{
		return const_cast<FInterpCurve<T>*>(const_cast<const UMetaSplineMetadata*>(this)->FindCurve<T>(InName));
	}

private:
	template<typename T, typename TMapType>
	void AddCurve(TMap<FName, TMapType>& Map, const FProperty* InProperty)
	{
		using TUnderlyingType = typename TCurveUnderlyingType<TMapType>::Type;

		InitializeCurve<T>(Map.Add(InProperty->GetFName(), {}), InProperty);
		NumCurves++;
	}

	template<typename T>
	void InitializeCurve(FInterpCurve<T>& Curve, const FProperty* InProperty)
	{
		T* Value = InProperty->ContainerPtrToValuePtr<T>(MetaClass->GetDefaultObject());

		for (int32 i = 0; i < NumPoints; i++)
		{
			Curve.AddPoint(i, *Value);
		}
	}

	template<typename TMapType, typename F>
	void TransformCurveMap(TMap<FName, TMapType>& Map, F&& Function)
	{
		for (auto& Curve : Map)
		{
			Function(Curve);
		}
	}

	template<typename F>
	void TransformCurves(F&& Function)
	{
		TransformCurveMap(FloatCurves, Function);
		TransformCurveMap(VectorCurves, Function);
	}

	template<typename F>
	void TransformPoints(F&& Function)
	{
		TransformCurves([Function](auto& Curve)
		{
			for (auto& Point : Curve.Value.Points)
			{
				Function(Point);
			}
		});
	}

	template<typename F>
	void TransformPointsIndex(F&& Function)
	{
		TransformCurves([Function](auto& Curve)
		{
			for (int32 i = 0; i < Curve.Value.Points.Num(); i++)
			{
				Function(Curve.Value.Points[i], i);
			}
		});
	}

	template<typename F>
	void TransformPoints(int32 StartIndex, F&& Function)
	{
		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Value.Points;
			for (int32 i = StartIndex; i < Points.Num(); i++)
			{
				Function(Points[i]);
			}
		});
	}

	template<typename F>
	void TransformPointsIndex(int32 StartIndex, F&& Function)
	{
		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Value.Points;
			for (int32 i = StartIndex; i < Points.Num(); i++)
			{
				Function(Points[i], i);
			}
		});
	}

	template<typename T>
	auto& FindCurveMapForType()
	{
		if constexpr (TIsSame<T, float>::Value)
			return FloatCurves;
		if constexpr (TIsSame<T, FVector>::Value)
			return VectorCurves;
	}

private:
	UPROPERTY(EditAnywhere, Category = "Meta")
	TMap<FName, FInterpCurveFloat> FloatCurves;

	UPROPERTY(EditAnywhere, Category = "Meta")
	TMap<FName, FInterpCurveVector> VectorCurves;

	UPROPERTY()
	TSubclassOf<UObject> MetaClass;

	int32 NumCurves = 0;
	int32 NumPoints = 0;

	friend class FMetaSplineMetadataDetails;
	template<typename T> friend struct FAddCurve;
};
