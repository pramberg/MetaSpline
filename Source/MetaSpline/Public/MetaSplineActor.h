// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetaSplineActor.generated.h"

UCLASS()
class METASPLINE_API AMetaSplineActor : public AActor
{
	GENERATED_BODY()
	
public:
	AMetaSplineActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UMetaSplineComponent* Spline;
};
