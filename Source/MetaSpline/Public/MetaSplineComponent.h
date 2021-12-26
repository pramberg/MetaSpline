// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "MetaSplineMetadata.h"
#include "MetaSplineComponent.generated.h"

class UMetaSplineMetadata;

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
	float GetMetadataFloatAtPoint(FName InProperty, int32 InIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtPoint(FName InProperty, int32 InIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	float GetMetadataFloatAtKey(FName InProperty, float InKey) const;

	UFUNCTION(BlueprintCallable, Category = "Spline|Metadata")
	FVector GetMetadataVectorAtKey(FName InProperty, float InKey) const;

public:
	// -- Overrides --
	virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;
	void ApplyComponentInstanceData(struct FMetaSplineInstanceData* ComponentInstanceData, const bool bPostUCS);
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	
	/** Ensures the metadata is up to date when the metadata class has been modified because the property was changed
	 * to another class, or it was recompiled.
	 *
	 * This function should always be safe to call, and won't do anything if nothing has changed.
	 */
	void RefreshMetadata();
#endif

	virtual USplineMetadata* GetSplinePointsMetadata() override { return Metadata; }
	virtual const USplineMetadata* GetSplinePointsMetadata() const override { return Metadata; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Metadata)
	TSubclassOf<UObject> MetadataClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Metadata)
	bool bDrawDebugMetadata = true;

private:
	void SynchronizeProperties();

private:
	UPROPERTY(Instanced)
	UMetaSplineMetadata* Metadata;

	static FProperty* MetadataProperty;
	static FProperty* ClosedLoopProperty;
	static FProperty* LoopPositionOverrideProperty;
	static FProperty* LoopPositionProperty;

	friend class FMetaSplineMetadataDetails;
};

USTRUCT()
struct FMetaSplineInstanceData : public FSplineInstanceData
{
	GENERATED_BODY()

public:
	FMetaSplineInstanceData() = default;
	explicit FMetaSplineInstanceData(const UMetaSplineComponent* SourceComponent) : FSplineInstanceData(SourceComponent)
	{
	}

	virtual ~FMetaSplineInstanceData() = default;
	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override;

	UPROPERTY()
	UMetaSplineMetadata* Metadata;

	// The default spline implementation doesn't handle these settings properly, and cause issues when using splines in construction scripts.
	// Luckily it can be fixed here, though it is a bit hacky since some properties are private.
	bool bClosedLoop;
	bool bClosedLoopPositionOverride;
	float ClosedLoopPosition;
};