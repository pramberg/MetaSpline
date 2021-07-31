// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include <UObject/UnrealType.h>
#include "MetaSplineMetadata.generated.h"

namespace CurveUnderlyingType_Private
{
	template<typename T> struct TCurveUnderlyingTypeImpl { using Type = void; };
	template<typename T> struct TCurveUnderlyingTypeImpl<FInterpCurve<T>> { using Type = T; };
	template<> struct TCurveUnderlyingTypeImpl<FInterpCurveFloat> { using Type = float; };
	template<> struct TCurveUnderlyingTypeImpl<FInterpCurveVector> { using Type = FVector; };
	template<> struct TCurveUnderlyingTypeImpl<FInterpCurveQuat> { using Type = FQuat; };
	template<> struct TCurveUnderlyingTypeImpl<FInterpCurveLinearColor> { using Type = FLinearColor; };
	template<> struct TCurveUnderlyingTypeImpl<FInterpCurveVector2D> { using Type = FVector2D; };
}

template<typename T>
struct TCurveUnderlyingType
{ 
	using Type = typename CurveUnderlyingType_Private::TCurveUnderlyingTypeImpl<typename TDecay<T>::Type>::Type;
};

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

	template<typename T> const FInterpCurve<T>* FindCurve(const FName InName) const { return nullptr; }
	template<> const FInterpCurve<float>* FindCurve<float>(const FName InName) const { return FloatCurves.Find(InName); }
	template<> const FInterpCurve<FVector>* FindCurve<FVector>(const FName InName) const { return VectorCurves.Find(InName); }

	template<typename T>
	FInterpCurve<T>* FindCurve(const FName InName)
	{
		return const_cast<FInterpCurve<T>*>(const_cast<const UMetaSplineMetadata*>(this)->FindCurve<T>(InName));
	}

private:
	template<typename T, typename TMapType>
	void AddCurve(TMap<FName, TMapType>& Map, const FProperty* InProperty)
	{
		auto& Curve = Map.Add(InProperty->GetFName(), {});

		const T& Value = *InProperty->ContainerPtrToValuePtr<T>(MetaClass->GetDefaultObject());
		for (int32 i = 0; i < NumPoints; i++)
		{
			Curve.AddPoint(i, Value);
		}

		NumCurves++;
	}

	template<typename TMapType, typename F>
	void TransformCurveMap(TMap<FName, TMapType>& Map, F&& Function)
	{
		for (auto& Curve : Map)
		{
			if constexpr (TIsInvocable<F, decltype(Curve.Value)>::Value)
			{
				Function(Curve.Value);
			}
			else if constexpr (TIsInvocable<F, decltype(Curve)>::Value)
			{
				Function(Curve);
			}
			else if constexpr (TIsInvocable<F, decltype(Curve.Key), decltype(Curve.Value)>::Value)
			{
				Function(Curve.Key, Curve.Value);
			}
			else
			{
				static_assert(false, "Invalid function passed to UMetaSplineMetadata::TransformCurveMap()");
			}
		}
	}

	template<typename F>
	void TransformCurves(F&& Function)
	{
		TransformCurveMap(FloatCurves, Forward<F>(Function));
		TransformCurveMap(VectorCurves, Forward<F>(Function));
	}

	template<typename F>
	void TransformPoints(int32 StartIndex, F&& Function)
	{
		TransformCurves([=](auto& Curve)
		{
			auto& Points = Curve.Points;
			for (int32 i = StartIndex; i < Points.Num(); i++)
			{
				if constexpr (TIsInvocable<F, decltype(Points[i])>::Value)
				{
					Function(Points[i]);
				}
				else if constexpr (TIsInvocable<F, decltype(Points[i]), int32>::Value)
				{
					Function(Points[i], i);
				}
				else
				{
					static_assert(false, "Invalid function passed to UMetaSplineMetadata::TransformPoints()");
				}
			}
		});
	}

	template<typename F>
	void TransformPoints(F&& Function)
	{
		TransformPoints(0, Forward<F>(Function));
	}

	template<typename T>
	auto& FindCurveMapForType()
	{
		if constexpr (TIsSame<T, float>::Value) { return FloatCurves; }
		else if constexpr (TIsSame<T, FVector>::Value) { return VectorCurves; }
		else { static_assert(false, "Curve type not supported!"); }
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
