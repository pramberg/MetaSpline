// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineMetadataDetails.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.h"
#include "MetaSplineTemplateHelpers.h"

#include <PropertyEditorModule.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>
#include <IDetailGroup.h>

#include <Widgets/Text/STextBlock.h>
#include <Widgets/Layout/SBorder.h>

#define LOCTEXT_NAMESPACE "MetaSplineMetadataDetails"

TSharedPtr<ISplineMetadataDetails> UMetaSplineMetadataDetailsFactory::Create()
{
	return MakeShared<FMetaSplineMetadataDetails>();
}

UClass* UMetaSplineMetadataDetailsFactory::GetMetadataClass() const
{
	return UMetaSplineMetadata::StaticClass();
}

FText FMetaSplineMetadataDetails::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Metadata");
}

void FMetaSplineMetadataDetails::Update(USplineComponent* InSplineComponent, const TSet<int32>& InSelectedKeys)
{
	SplineComp = InSplineComponent;
	SelectedKeys = InSelectedKeys;

	if (!InSplineComponent)
	{
		return;
	}

	if (UMetaSplineComponent* MetaSpline = Cast<UMetaSplineComponent>(InSplineComponent))
	{
		if (MetaSpline->MetadataClass != MetaClass)
		{
			MetaClass = MetaSpline->MetadataClass;
			MetaClassInstances.Empty();
		}

		// Make sure the number of instances match the number of selected points
		while (MetaClassInstances.Num() < InSelectedKeys.Num())
		{
			MetaClassInstances.Emplace(MetaClass ? NewObject<UObject>(MetaSpline, *MetaClass) : nullptr);
		}

		if (MetaClassInstances.Num() > InSelectedKeys.Num())
		{
			MetaClassInstances.RemoveAtSwap(InSelectedKeys.Num(), MetaClassInstances.Num() - InSelectedKeys.Num());
		}

		DetailsView->SetObjects(MetaClassInstances);
	}

	if (UMetaSplineMetadata* Metadata = Cast<UMetaSplineMetadata>(InSplineComponent->GetSplinePointsMetadata()))
	{
		Metadata->TransformCurves([this, &InSelectedKeys](FName Key, auto& Curve)
		{
			const auto& Points = Curve.Points;

			TArray<UObject*>::TIterator It = MetaClassInstances.CreateIterator();
			const FProperty* Property = MetaClass->FindPropertyByName(Key);
			for (int32 Index : InSelectedKeys)
			{
				using TUnderlyingType = TCurveUnderlyingType<decltype(Curve)>::Type;
				*Property->ContainerPtrToValuePtr<TUnderlyingType>(*It) = Points[Index].OutVal;
				++It;
			}
		});
	}
}

void FMetaSplineMetadataDetails::GenerateChildContent(IDetailGroup& InGroup)
{
	FDetailsViewArgs Args;
	Args.bShowActorLabel = false;
	Args.bAllowMultipleTopLevelObjects = false;
	Args.bHideSelectionTip = true;
	Args.bAllowSearch = false;
	Args.bLockable = false;
	Args.bAllowFavoriteSystem = false;
	Args.bShowOptions = false;
	Args.bShowScrollBar = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	Args.bCustomNameAreaLocation = false;
	Args.bShowDifferingPropertiesOption = false;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyEditorModule.CreateDetailView(Args);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FMetaSplineMetadataDetails::OnFinishedChangingProperties);
	const auto VisibilityAttr = TAttribute<EVisibility>::Create([this]() { return MetaClass ? EVisibility::Visible : EVisibility::Collapsed; });

	InGroup.HeaderRow()
		.Visibility(VisibilityAttr)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::Create([this]() { return FText::Format(LOCTEXT("FormattedHeader", "Metadata - {0}"), MetaClass->GetDisplayNameText()); }))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
			.Visibility(VisibilityAttr)
		];

	InGroup.AddWidgetRow()
		.Visibility(VisibilityAttr)
		[
			SNew(SBox)
			.Padding(FMargin(0.0f, 12.0f, 0.0f, 12.0f))
			.RenderTransform(FSlateRenderTransform(FVector2D(-12.0f, 0.0f))) // In 4.26.2 there is a weird little invisible (but not collapsed) arrow that indents the widget too much, so unindent a little bit here.
			[
				SNew(SBorder)
				.Padding(4.0f)
				[
					SNew(SBox)
					.Padding(4.0f)
					[
						DetailsView.ToSharedRef()
					]
				]
			]
		];
}

void FMetaSplineMetadataDetails::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(MetaClassInstances);
	Collector.AddReferencedObject(SplineComp);
}

class UMetaSplineMetadata* FMetaSplineMetadataDetails::GetMetadata() const
{
	return SplineComp ? Cast<UMetaSplineMetadata>(SplineComp->GetSplinePointsMetadata()) : nullptr;
}

// Wrapper struct that can be passed to FMetaSplineTemplateHelpers::ExecuteOnProperty, that updates the FInterpCurve for
// a modified property.
template<typename T>
struct FUpdateMetadata
{
	static void Execute(FMetaSplineMetadataDetails& InOutDetails, UMetaSplineMetadata& InOutMetadata, const FPropertyChangedEvent& InProperty)
	{
		auto* Curve = InOutMetadata.FindCurve<T>(InProperty.MemberProperty->GetFName());
		if (!Curve)
		{
			// #TODO: Make sure this doesn't happen...
			UE_LOG(LogTemp, Error, TEXT("Couldn't find curve to modify. Please refresh the metadata class"));
			return;
		}

		USplineComponent* Spline = InOutDetails.SplineComp;
		const TSet<int32>& SelectedKeys = InOutDetails.SelectedKeys;

		Spline->GetSplinePointsMetadata()->Modify();

		TArray<UObject*>::TConstIterator It = InOutDetails.GetMetaClassInstances()->CreateConstIterator();
		for (int32 Index : SelectedKeys)
		{
			Curve->Points[Index].OutVal = *InProperty.MemberProperty->ContainerPtrToValuePtr<T>(*It);
			++It;
		}

		Spline->UpdateSpline();
		Spline->bSplineHasBeenEdited = true;

		InOutDetails.Update(Spline, SelectedKeys);
	}
};

void FMetaSplineMetadataDetails::OnFinishedChangingProperties(const FPropertyChangedEvent& InProperty)
{
	if (!GetMetadata())
		return;
	
	FMetaSplineTemplateHelpers::ExecuteOnProperty<FUpdateMetadata>(InProperty.MemberProperty, *this, *GetMetadata(), InProperty);
}

#undef LOCTEXT_NAMESPACE