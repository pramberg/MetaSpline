// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.generated.h"

/**
 * 
 */
UCLASS()
class METASPLINE_API UMetaSplineComponent : public USplineComponent
{
	GENERATED_BODY()
	
public:
	UMetaSplineComponent();

	virtual USplineMetadata* GetSplinePointsMetadata() override { return Metadata; }
	virtual const USplineMetadata* GetSplinePointsMetadata() const override { return Metadata; }


	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Metadata)
	TSubclassOf<UObject> MetadataClass;


	virtual void PostLoad() override;


	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;


	virtual void PostEditImport() override;

	UFUNCTION(BlueprintCallable, Category="Spline|Metadata")
	float GetMetadataFloatAtPoint(FName InProperty, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtPoint(FName InProperty, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	float GetMetadataFloatAtKey(FName InProperty, float InKey);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtKey(FName InProperty, float InKey);

	virtual void UpdateSpline() override;

private:
	UPROPERTY()
	class UMetaSplineMetadata* Metadata;
};
