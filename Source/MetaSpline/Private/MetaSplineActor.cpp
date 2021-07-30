// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineActor.h"
#include "MetaSplineComponent.h"

AMetaSplineActor::AMetaSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<UMetaSplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
}