// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "SplineMetadataDetailsFactory.h"
#include <ScopedTransaction.h>
#include <UObject/StrongObjectPtr.h>
#include "MetaSplineMetadataDetails.generated.h"

UCLASS()
class UMetaSplineMetadataDetailsFactory : public USplineMetadataDetailsFactoryBase
{
	GENERATED_BODY()
	
public:
	virtual TSharedPtr<ISplineMetadataDetails> Create() override;
	virtual UClass* GetMetadataClass() const override;
};

class FMetaSplineMetadataDetails : public FGCObject, public ISplineMetadataDetails, public TSharedFromThis<FMetaSplineMetadataDetails>
{
public:
	virtual ~FMetaSplineMetadataDetails() override {}
	virtual FName GetName() const override { return FName(TEXT("MetaSplineMetadataDetails")); }
	virtual FText GetDisplayName() const override;
	virtual void Update(USplineComponent* InSplineComponent, const TSet<int32>& InSelectedKeys) override;
	virtual void GenerateChildContent(IDetailGroup& InGroup) override;

	TWeakObjectPtr<USplineComponent> SplineComp = nullptr;
	TSet<int32> SelectedKeys;

	TArray<UObject*>* GetMetaClassInstances() { return &MetaClassInstances; }

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	EVisibility IsEnabled() const { return (SelectedKeys.Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed; }
	bool IsOnePointSelected() const { return SelectedKeys.Num() == 1; }

	class UMetaSplineMetadata* GetMetadata() const;

	void OnFinishedChangingProperties(const FPropertyChangedEvent& InProperty);
private:

	TSharedPtr<class IDetailsView> DetailsView = nullptr;

	TSubclassOf<UObject> MetaClass;

	// We use an array of instances of the meta class to allow the details view widget to do as much work for us as possible.
	// In this case, if we edit multiple points, it will automatically handle points with different settings, and display the appropriate value.
	TArray<UObject*> MetaClassInstances;
};
