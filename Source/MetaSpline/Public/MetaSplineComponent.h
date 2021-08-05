// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.generated.h"

/**
 * A spline component with a simple interface for adding metadata to spline points.
 */
UCLASS(ClassGroup = Utility, BlueprintType, meta = (BlueprintSpawnableComponent))
class METASPLINE_API UMetaSplineComponent : public USplineComponent
{
	GENERATED_BODY()
	
public:
	UMetaSplineComponent();

	// -- New metadata accessors --
	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	float GetMetadataFloatAtPoint(FName InProperty, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtPoint(FName InProperty, int32 InIndex);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	float GetMetadataFloatAtKey(FName InProperty, float InKey);

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtKey(FName InProperty, float InKey);

public:
	// -- Overrides --
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	virtual USplineMetadata* GetSplinePointsMetadata() override { return Metadata; }
	virtual const USplineMetadata* GetSplinePointsMetadata() const override { return Metadata; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Metadata)
	TSubclassOf<UObject> MetadataClass;

private:
	UPROPERTY()
	class UMetaSplineMetadata* Metadata;
};
