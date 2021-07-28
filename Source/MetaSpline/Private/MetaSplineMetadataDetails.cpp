// Fill out your copyright notice in the Description page of Project Settings.


#include "MetaSplineMetadataDetails.h"
#include "MetaSplineMetadata.h"
#include <DetailLayoutBuilder.h>
#include <Widgets/Input/SNumericEntryBox.h>
#include <Widgets/Text/STextBlock.h>
#include <IDetailGroup.h>
#include <DetailWidgetRow.h>
#include <UMGEditor/Public/Components/DetailsView.h>
#include <UMGEditor/Public/Components/SinglePropertyView.h>
#include <PropertyEditorModule.h>
#include <ISinglePropertyView.h>
#include "MetaSplineComponent.h"
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

template<class T>
bool UpdateMultipleValue(TOptional<T>& CurrentValue, T InValue)
{
	if (!CurrentValue.IsSet())
	{
		CurrentValue = InValue;
	}
	else if (CurrentValue.IsSet() && CurrentValue.GetValue() != InValue)
	{
		CurrentValue.Reset();
		return false;
	}

	return true;
}

void FMetaSplineMetadataDetails::Update(USplineComponent* InSplineComponent, const TSet<int32>& InSelectedKeys)
{
	SplineComp = InSplineComponent;
	SelectedKeys = InSelectedKeys;

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

	if (InSplineComponent)
	{
		if (UMetaSplineMetadata* Metadata = Cast<UMetaSplineMetadata>(InSplineComponent->GetSplinePointsMetadata()))
		{
			Metadata->TransformCurves([this, &InSelectedKeys](auto& Curve)
			{
				auto& Points = Curve.Value.Points;

				using TPointsArray = typename TDecay<decltype(Points)>::Type;
				using TPoint = typename TPointsArray::ElementType;
				using TUnderlyingType = typename TCurvePointUnderlyingType<TPoint>::Type;

				TOptional<TUnderlyingType> Optional;
				bool bUpdateValue = true;

				TArray<UObject*>::TIterator It = MetaClassInstances.CreateIterator();
				FProperty* Property = MetaClass->FindPropertyByName(Curve.Key);
				for (int32 Index : InSelectedKeys)
				{
					*Property->ContainerPtrToValuePtr<TUnderlyingType>(*It) = Points[Index].OutVal;
					++It;
				}
			});
			
		}
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
			SAssignNew(RootWidget, SBox)
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
}

class UMetaSplineMetadata* FMetaSplineMetadataDetails::GetMetadata() const
{
	return SplineComp ? Cast<UMetaSplineMetadata>(SplineComp->GetSplinePointsMetadata()) : nullptr;
}

template<typename TValue, typename TCurve>
void SetValues(FMetaSplineMetadataDetails& Details, UMetaSplineMetadata& Metadata, const FPropertyChangedEvent& InProperty)
{
	auto* Curve = Metadata.FindCurve<TCurve>(InProperty.MemberProperty->GetFName());
	if (!Curve)
	{
		UE_LOG(LogTemp, Error, TEXT("Something has gone horribly wrong :pepehands:"));
		return;
	}

	Details.SplineComp->GetSplinePointsMetadata()->Modify();
	TArray<UObject*>::TConstIterator It = Details.GetMetaClassInstances()->CreateConstIterator();
	for (int32 Index : Details.SelectedKeys)
	{
		Curve->Points[Index].OutVal = *InProperty.MemberProperty->ContainerPtrToValuePtr<TValue>(*It);
		++It;
	}

	Details.SplineComp->UpdateSpline();
	Details.SplineComp->bSplineHasBeenEdited = true;
	Details.Update(Details.SplineComp, Details.SelectedKeys);
}

void FMetaSplineMetadataDetails::OnFinishedChangingProperties(const FPropertyChangedEvent& InProperty)
{
	UE_LOG(LogTemp, Display, TEXT("Changed Property: %s"), *InProperty.GetPropertyName().ToString());
	
	if (!GetMetadata())
		return;
	
	const FName Type = FName(InProperty.MemberProperty->GetCPPType());

	auto& Metadata = *GetMetadata();
	if (Type == TEXT("float"))
		SetValues<float, FInterpCurveFloat>(*this, Metadata, InProperty);
	else if (Type == TEXT("FVector"))
		SetValues<FVector, FInterpCurveVector>(*this, Metadata, InProperty);
	//GetMetadata()->OnMetadataChanged(InProperty);
}

#undef LOCTEXT_NAMESPACE