// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetaSplineActor.generated.h"

UCLASS()
class METASPLINE_API AMetaSplineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMetaSplineActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UMetaSplineComponent* Spline;
};
