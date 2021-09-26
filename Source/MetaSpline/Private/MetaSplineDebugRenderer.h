// Copyright(c) 2021 Viktor Pramberg
#pragma once
#include "CoreMinimal.h"

class UCanvas;

struct FMetaSplineDebugInfo
{
	FText Text;
	FVector ScreenPosition;
};

class FMetaSplineDebugRenderer
{
public:
	~FMetaSplineDebugRenderer();

private:
	FMetaSplineDebugRenderer();

	void Draw(UCanvas* Canvas, class APlayerController*);
	TArray<FMetaSplineDebugInfo> CollectInfosFromSpline(UCanvas* Canvas, class UMetaSplineComponent* Spline);

private:
	const TSharedRef<class FSlateFontMeasure> FontMeasure;

	class FDelegateHandle DelegateHandle;

	friend class FMetaSplineModule;
};