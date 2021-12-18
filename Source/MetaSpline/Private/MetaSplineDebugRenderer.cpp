// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineDebugRenderer.h"
#include "MetaSplineComponent.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineSettings.h"

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
			ScreenPosition.X > Canvas->CachedDisplayWidth || ScreenPosition.Y > Canvas->CachedDisplayHeight)
		{
			continue;
		}

		FTextBuilder Builder;
		Metadata->TransformCurves([&](FName Key, auto Curve)
		{
			FFormatOrderedArguments Args;
			Args.Add(FText::FromName(Key));

			auto& Value = Curve.Points[i].OutVal;
			using TUnderlyingType = TCurveUnderlyingType<decltype(Curve)>::Type;
			if constexpr (TIsFundamentalType<TUnderlyingType>::Value)
			{
				Args.Add(Value);
			}
			else
			{
				Args.Add(FText::FromString(Value.ToString()));
			}

			Builder.AppendLine(FText::Format(LOCTEXT("FormattedDebugInfo", "{0}: {1}"), Args));
		});

		Infos.Add({ Builder.ToText(), ScreenPosition });
	}

	return Infos;
}

#undef LOCTEXT_NAMESPACE