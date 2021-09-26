// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineComponent.h"

UMetaSplineComponent::UMetaSplineComponent()
{
	Metadata = CreateDefaultSubobject<UMetaSplineMetadata>(TEXT("Metadata"));
}

// -- New metadata accessors --
template<class T>
T GetPropertyValueAtKey(const UMetaSplineMetadata* Metadata, float InKey, FName PropertyName)
{
	if (Metadata)
	{
		if (auto* Curve = Metadata->FindCurve<T>(PropertyName))
		{ 
			return Curve->Eval(InKey);
		}
	}
	return T();
}

float UMetaSplineComponent::GetMetadataFloatAtPoint(FName InProperty, int32 InIndex) const
{
	return GetMetadataFloatAtKey(InProperty, static_cast<float>(InIndex));
}

FVector UMetaSplineComponent::GetMetadataVectorAtPoint(FName InProperty, int32 InIndex) const
{
	return GetMetadataVectorAtKey(InProperty, static_cast<float>(InIndex));
}

float UMetaSplineComponent::GetMetadataFloatAtKey(FName InProperty, float InKey) const
{
	return GetPropertyValueAtKey<float>(Metadata, InKey, InProperty);
}

FVector UMetaSplineComponent::GetMetadataVectorAtKey(FName InProperty, float InKey) const
{
	return GetPropertyValueAtKey<FVector>(Metadata, InKey, InProperty);
}

// -- Overrides --
void UMetaSplineComponent::PostLoad()
{
	Super::PostLoad();
	
	if (Metadata)
	{
		if (!Metadata->HasValidMetadataClass())
		{
			Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
		}
		Metadata->Fixup(GetNumberOfSplinePoints(), this);
	}
}

#if WITH_EDITOR
void UMetaSplineComponent::PostEditImport()
{
	Super::PostEditImport();

	if (Metadata)
	{
		Metadata->Fixup(GetNumberOfSplinePoints(), this);
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

void UMetaSplineComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMetaSplineComponent, MetadataClass))
	{
		Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
	}
}
#endif