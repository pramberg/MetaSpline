// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include <UObject/UnrealType.h>
#include "MetaSplineMetadata.generated.h"

template<typename T>
constexpr bool SupportsToString = false;

template<>
constexpr bool SupportsToString<FVector> = true;

// Finds the type T of a FInterpCurvePoint<T>
template<typename T> struct TCurvePointUnderlyingType { using Type = void; };
template<typename T> struct TCurvePointUnderlyingType<FInterpCurvePoint<T>> { using Type = T; };

template<typename T> struct TCurveUnderlyingType { using Type = void; };
template<typename T> struct TCurveUnderlyingType<FInterpCurve<T>> { using Type = T; };

/**
 *
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
	const T* FindCurve(const FName InName) const
	{
		if constexpr (TIsSame<T, FInterpCurveFloat>::Value)
			return FloatCurves.Find(InName);
		else if constexpr (TIsSame<T, FInterpCurveVector>::Value)
			return VectorCurves.Find(InName);
		return nullptr;
	}

	template<typename T>
	T* FindCurve(const FName InName)
	{
		if constexpr (TIsSame<T, FInterpCurveFloat>::Value)
			return FloatCurves.Find(InName);
		else if constexpr (TIsSame<T, FInterpCurveVector>::Value)
			return VectorCurves.Find(InName);
		return nullptr;
	}

private:
	template<typename T, typename TMapType>
	void AddCurve(TMap<FName, TMapType>& Map, FProperty* InProperty)
	{
		using TPointsArray = typename TDecay<decltype(Map)>::Type;
		using TPoint = typename TPointsArray::ElementType;
		using TUnderlyingType = typename TCurvePointUnderlyingType<TPoint>::Type;

		InitializeCurve<T>(Map.Add(InProperty->GetFName(), {}), InProperty);
		NumCurves++;
	}

	template<typename T>
	void InitializeCurve(FInterpCurve<T>& Curve, FProperty* InProperty)
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

	

private:
	UPROPERTY(EditAnywhere, Category = "Meta")
	TMap<FName, FInterpCurveFloat> FloatCurves;

	UPROPERTY(EditAnywhere, Category = "Meta")
	TMap<FName, FInterpCurveVector> VectorCurves;

	UPROPERTY()
	TSubclassOf<UObject> MetaClass;

	int32 NumCurves = 0;
	int32 NumPoints = 0;

	/*UPROPERTY(EditAnywhere, Category = "Water")
	TMap<FName, FInterpCurveVector> VectorCurves;*/

	friend class FMetaSplineMetadataDetails;
};
