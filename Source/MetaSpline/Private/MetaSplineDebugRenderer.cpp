// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineDebugRenderer.h"
#include "MetaSplineComponent.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineSettings.h"
#include "MetaSplineTemplateHelpers.h"

#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "MetaSplineDebugRenderer"

FMetaSplineDebugRenderer::FMetaSplineDebugRenderer() : FontMeasure(FSlateApplication::Get().GetRenderer()->GetFontMeasureService())
{
	DelegateHandle = UDebugDrawService::Register(TEXT("Splines"), FDebugDrawDelegate::CreateRaw(this, &FMetaSplineDebugRenderer::Draw));
}

FMetaSplineDebugRenderer::~FMetaSplineDebugRenderer()
{
	UDebugDrawService::Unregister(DelegateHandle);
}

void FMetaSplineDebugRenderer::Draw(class UCanvas* Canvas, class APlayerController*)
{
	const FConvexVolume& Frustum = Canvas->SceneView->ViewFrustum;
	UWorld* World = Canvas->SceneView->Family->Scene->GetWorld();

	TArray<FMetaSplineDebugInfo> CurrentFrameInfos;
	for (UMetaSplineComponent* Component : TObjectRange<UMetaSplineComponent>(RF_ClassDefaultObject, true, EInternalObjectFlags::PendingKill))
	{
		if (Component->GetWorld() != World)
		{
			continue;
		}

		if (!Component->bDrawDebug || !Component->bDrawDebugMetadata)
		{
			continue;
		}

		if (!Frustum.IntersectSphere(Component->Bounds.Origin, Component->Bounds.SphereRadius))
		{
			continue;
		}

		CurrentFrameInfos.Append(CollectInfosFromSpline(Canvas, Component));
	}

	if (CurrentFrameInfos.Num() == 0)
	{
		return;
	}

	// Make sure we draw items back to front
	CurrentFrameInfos.Sort([](const FMetaSplineDebugInfo& A, const FMetaSplineDebugInfo& B) { return A.ScreenPosition.Z < B.ScreenPosition.Z; });

	const UMetaSplineUserSettings* Settings = GetDefault<UMetaSplineUserSettings>();
	if (Settings->NumberOfPoints >= 0)
	{
		CurrentFrameInfos.RemoveAt(0, FMath::Min(FMath::Max(CurrentFrameInfos.Num() - Settings->NumberOfPoints, 0), CurrentFrameInfos.Num()));
	}

	UFont* Font(GEngine->GetTinyFont());
	const FSlateFontInfo& FontInfo(Font->GetLegacySlateFontInfo());

	for (FMetaSplineDebugInfo& Info : CurrentFrameInfos)
	{
		const FVector2D DrawPosition = FVector2D(Info.ScreenPosition) + 10.0f;

		const FVector2D TextSize = FontMeasure->Measure(Info.Text, FontInfo);
		const FVector2D BoxSize = TextSize + 10.0f;

		const FVector2D PaddingDiff = BoxSize - TextSize;
		const FVector2D BoxPosition = DrawPosition - PaddingDiff * 0.5f;

		{
			FCanvasTileItem Background(BoxPosition, BoxSize, FLinearColor::Black.CopyWithNewOpacity(0.5f));
			Background.BlendMode = SE_BLEND_Translucent;

			Canvas->DrawItem(Background);
		}

		{
			FCanvasBoxItem Border(BoxPosition, BoxSize);
			Border.LineThickness = 1.0f;
			Border.SetColor(FLinearColor::Gray.CopyWithNewOpacity(0.75f));
			Canvas->DrawItem(Border);
		}

		{
			FCanvasLineItem Line(FVector2D(Info.ScreenPosition), BoxPosition);
			Line.LineThickness = 1.0f;
			Line.SetColor(FLinearColor::Gray.CopyWithNewOpacity(0.75f));
			Canvas->DrawItem(Line);
		}

		{
			FCanvasTextItem Text(DrawPosition, Info.Text, Font, FLinearColor::White);
			Text.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(Text);
		}
	}
}

template<typename T>
struct FCollectInfoFromProperty
{
	static FText Execute(UMetaSplineMetadata& InOutMetadata, const FProperty* InProperty, int32 InIndex)
	{
		const auto* Curve = InOutMetadata.FindCurve<T>(InProperty->GetFName());
		if (!Curve)
		{
			return FText();
		}
		
		FFormatOrderedArguments Args;
		Args.Add(InProperty->GetDisplayNameText());
		
		const T& Value = Curve->Points[InIndex].OutVal;
		if constexpr (TIsFundamentalType<T>::Value)
		{
			Args.Add(Value);
		}
		else
		{
			Args.Add(FText::FromString(Value.ToString()));
		}

		return FText::Format(LOCTEXT("FormattedDebugInfo", "{0}: {1}"), Args);
	}
};

TArray<FMetaSplineDebugInfo> FMetaSplineDebugRenderer::CollectInfosFromSpline(UCanvas* Canvas, UMetaSplineComponent* Spline)
{
	TArray<FMetaSplineDebugInfo> Infos;

	if (!Spline)
	{
		return Infos;
	}

	UMetaSplineMetadata* Metadata = Cast<UMetaSplineMetadata>(Spline->GetSplinePointsMetadata());

	if (!Metadata || !Metadata->MetaClass)
	{
		return Infos;
	}

	check(Spline->GetNumberOfSplinePoints() == Metadata->NumPoints);

	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
	{
		const FVector WorldPosition = Spline->GetWorldLocationAtSplinePoint(i);
		const FVector ScreenPosition = Canvas->Project(WorldPosition);

		if (ScreenPosition.Z <= 0.0f || ScreenPosition.X < 0.0f || ScreenPosition.Y < 0.0f ||
			ScreenPosition.X > Canvas->SizeX || ScreenPosition.Y > Canvas->SizeY)
		{
			continue;
		}

		FTextBuilder Builder;
		for (const FProperty* Prop : TFieldRange<FProperty>(Metadata->MetaClass))
		{
			// It's probably not optimal to do it in this order. An optimization would be to iterate over the property in the
			// outer loop.
			Builder.AppendLine(
				FMetaSplineTemplateHelpers::ExecuteOnProperty<FCollectInfoFromProperty>(Prop, *Metadata, Prop, i)
			);
		}

		Infos.Add({ Builder.ToText(), ScreenPosition });
	}

	return Infos;
}

#undef LOCTEXT_NAMESPACE