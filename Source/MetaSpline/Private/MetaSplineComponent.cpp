// Fill out your copyright notice in the Description page of Project Settings.


#include "MetaSplineComponent.h"

UMetaSplineComponent::UMetaSplineComponent()
{
	Metadata = CreateDefaultSubobject<UMetaSplineMetadata>(TEXT("Metadata"));
}

void UMetaSplineComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMetaSplineComponent, MetadataClass))
	{
		Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
	}
}

void UMetaSplineComponent::PostLoad()
{
	Super::PostLoad();
	
	if (Metadata)
	{
		if (!Metadata->HasValidMetadataClass())
			Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
		Metadata->Fixup(GetNumberOfSplinePoints(), this);
	}
	else if (MetadataClass)
	{
		
	}
}

void UMetaSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Metadata)
	{
		Metadata->Fixup(GetNumberOfSplinePoints(), this);
	}
}

void UMetaSplineComponent::PostEditImport()
{
	Super::PostEditImport();

	if (Metadata)
	{
		Metadata->Fixup(GetNumberOfSplinePoints(), this);
	}
}

template<class T>
T GetPropertyValueAtSplinePoint(const UMetaSplineMetadata* Metadata, float InKey, FName PropertyName)
{
	if (Metadata)
	{
		if constexpr (TIsSame<T, float>::Value)
			if (auto* Curve = Metadata->FindCurve<FInterpCurveFloat>(PropertyName)) { return Curve->Eval(InKey); }
		else if constexpr (TIsSame<T, FVector>::Value)
			if (auto* Curve = Metadata->FindCurve<FInterpCurveVector>(PropertyName)) { return Curve->Eval(InKey); }
			
	}

	return T();
}

float UMetaSplineComponent::GetMetadataFloatAtPoint(FName InProperty, int32 InIndex)
{
	return GetMetadataFloatAtKey(InProperty, static_cast<float>(InIndex));
}

FVector UMetaSplineComponent::GetMetadataVectorAtPoint(FName InProperty, int32 InIndex)
{
	return GetMetadataVectorAtKey(InProperty, static_cast<float>(InIndex));
}

float UMetaSplineComponent::GetMetadataFloatAtKey(FName InProperty, float InKey)
{
	return GetPropertyValueAtSplinePoint<float>(Metadata, InKey, InProperty);
}

FVector UMetaSplineComponent::GetMetadataVectorAtKey(FName InProperty, float InKey)
{
	return GetPropertyValueAtSplinePoint<FVector>(Metadata, InKey, InProperty);
	
}

void UMetaSplineComponent::UpdateSpline()
{
	Super::UpdateSpline();
	//Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
}
