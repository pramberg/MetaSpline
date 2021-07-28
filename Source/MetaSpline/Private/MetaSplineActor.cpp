// Fill out your copyright notice in the Description page of Project Settings.
#include "MetaSplineActor.h"
#include "MetaSplineComponent.h"

// Sets default values
AMetaSplineActor::AMetaSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<UMetaSplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);
}