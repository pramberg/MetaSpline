// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineComponent.h"

FProperty* UMetaSplineComponent::MetadataProperty = FindFProperty<FProperty>(UMetaSplineComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UMetaSplineComponent, Metadata));
FProperty* UMetaSplineComponent::ClosedLoopProperty = FindFProperty<FProperty>(USplineComponent::StaticClass(), FName(TEXT("bClosedLoop")));
FProperty* UMetaSplineComponent::LoopPositionOverrideProperty = FindFProperty<FProperty>(USplineComponent::StaticClass(), FName(TEXT("bLoopPositionOverride")));
FProperty* UMetaSplineComponent::LoopPositionProperty = FindFProperty<FProperty>(USplineComponent::StaticClass(), FName(TEXT("LoopPosition")));

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
	return GetPropertyValueAtKey<double>(Metadata, InKey, InProperty);
}

FVector UMetaSplineComponent::GetMetadataVectorAtKey(FName InProperty, float InKey) const
{
	return GetPropertyValueAtKey<FVector>(Metadata, InKey, InProperty);
}

// -- Overrides --
TStructOnScope<FActorComponentInstanceData> UMetaSplineComponent::GetComponentInstanceData() const
{
	TStructOnScope<FActorComponentInstanceData> InstanceData = MakeStructOnScope<FActorComponentInstanceData, FMetaSplineInstanceData>(this);
	FMetaSplineInstanceData* SplineInstanceData = InstanceData.Cast<FMetaSplineInstanceData>();

	if (bSplineHasBeenEdited)
	{
		SplineInstanceData->Metadata = Metadata;
		SplineInstanceData->SplineCurves = SplineCurves;
		SplineInstanceData->bClosedLoop = IsClosedLoop();

		check(LoopPositionOverrideProperty && LoopPositionProperty);

		SplineInstanceData->bClosedLoopPositionOverride = *LoopPositionOverrideProperty->ContainerPtrToValuePtr<bool>(this);
		SplineInstanceData->ClosedLoopPosition = *LoopPositionProperty->ContainerPtrToValuePtr<float>(this);
	}

	SplineInstanceData->bSplineHasBeenEdited = bSplineHasBeenEdited;

	return InstanceData;
}

void UMetaSplineComponent::ApplyComponentInstanceData(struct FMetaSplineInstanceData* ComponentInstanceData, const bool bPostUCS)
{
	check(ComponentInstanceData);

	if (bPostUCS)
	{
		if (bInputSplinePointsToConstructionScript)
		{
			// Don't reapply the saved state after the UCS has run if we are inputting the points to it.
			// This allows the UCS to work on the edited points and make its own changes.
			return;
		}
		else
		{
			static const TArray<FProperty*> Properties
			{
				MetadataProperty,
				ClosedLoopProperty,
				LoopPositionOverrideProperty,
				LoopPositionProperty,
			};
			RemoveUCSModifiedProperties(Properties);
		}
	}

	if (ComponentInstanceData->bSplineHasBeenEdited)
	{
		// Copy metadata to current component
		if (Metadata && ComponentInstanceData->Metadata)
		{
			Metadata->Modify();
			UEngine::CopyPropertiesForUnrelatedObjects(ComponentInstanceData->Metadata, Metadata);
		}

		SetClosedLoop(ComponentInstanceData->bClosedLoop);

		check(LoopPositionOverrideProperty && LoopPositionProperty);

		*LoopPositionOverrideProperty->ContainerPtrToValuePtr<bool>(this) = ComponentInstanceData->bClosedLoopPositionOverride;
		*LoopPositionProperty->ContainerPtrToValuePtr<float>(this) = ComponentInstanceData->ClosedLoopPosition;

		bModifiedByConstructionScript = false;
	}

	UpdateSpline();

	SynchronizeProperties();
}

void UMetaSplineComponent::PostLoad()
{
	Super::PostLoad();
	
	if (Metadata)
	{
		if (!Metadata->HasValidMetadataClass())
		{
			Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
		}
		SynchronizeProperties();
	}
}

#if WITH_EDITOR
void UMetaSplineComponent::PostEditImport()
{
	Super::PostEditImport();

	SynchronizeProperties();
}

void UMetaSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SynchronizeProperties();
}

void UMetaSplineComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	static const FName MetadataClassName = GET_MEMBER_NAME_CHECKED(UMetaSplineComponent, MetadataClass);
	if (PropertyChangedEvent.GetPropertyName() == MetadataClassName)
	{
		RefreshMetadata();
	}
}

void UMetaSplineComponent::RefreshMetadata()
{
	check(Metadata);
	Metadata->UpdateMetadataClass(MetadataClass ? MetadataClass.Get() : nullptr);
	UpdateSpline();
	SynchronizeProperties();
}
#endif

void UMetaSplineComponent::SynchronizeProperties()
{
	if (Metadata)
	{
		Metadata->Fixup(GetNumberOfSplinePoints(), this);

		Metadata->TransformCurves([this](auto& InOutCurve)
		{
			if (IsClosedLoop())
			{
				const float LastKey = InOutCurve.Points.Num() > 0 ? InOutCurve.Points.Last().InVal : 0.0f;

				// NOTE: This is all incredibly stupid and should not have to be done. For some reason these variables are private instead of protected,
				// and there is no way of accessing them except like this.
				check(LoopPositionOverrideProperty && LoopPositionProperty);

				const bool bLocalLoopPositionOverride = *LoopPositionOverrideProperty->ContainerPtrToValuePtr<bool>(this);
				const float LocalLoopPosition = *LoopPositionProperty->ContainerPtrToValuePtr<float>(this);

				const float LoopKey = bLocalLoopPositionOverride ? LocalLoopPosition : LastKey + 1.0f;
				InOutCurve.SetLoopKey(LoopKey);
			}
			else
			{
				InOutCurve.ClearLoopKey();
			}

			InOutCurve.AutoSetTangents(0.0f, bStationaryEndpoints);
		});
	}
}

void FMetaSplineInstanceData::ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase)
{
	if (UMetaSplineComponent* SplineComp = CastChecked<UMetaSplineComponent>(Component))
	{
		// This ensures there is no stale data causing issues where the spline is marked as read-only even though it shouldn't.
		// There might be a better solution, but this works.
		SplineComp->UpdateSpline();

		Super::ApplyToComponent(Component, CacheApplyPhase);
		SplineComp->ApplyComponentInstanceData(this, (CacheApplyPhase == ECacheApplyPhase::PostUserConstructionScript));
	}
}
