// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineEditor.h"
#include "MetaSplineComponent.h"
#include "MetaSplineTemplateHelpers.h"

#include "BlueprintCompilationManager.h"

void FMetaSplineEditorModule::HandleBlueprintPreCompile(UBlueprint* InBlueprint)
{
	for (auto* SplineComp : TObjectRange<UMetaSplineComponent>())
	{
		if (!SplineComp || !SplineComp->GetWorld() || !SplineComp->MetadataClass)
		{
			continue;
		}

		if (InBlueprint != SplineComp->MetadataClass->ClassGeneratedBy)
		{
			continue;
		}

		if (UMetaSplineMetadata* Metadata = Cast<UMetaSplineMetadata>(SplineComp->GetSplinePointsMetadata()))
		{
			Metadata->UpdateValuesOnUnmodifiedPoints();
			//SplineComp->RefreshMetadata();
		}
	}
}

void FMetaSplineEditorModule::HandleBlueprintCompiled()
{
	for (auto* SplineComp : TObjectRange<UMetaSplineComponent>())
	{
		if (!SplineComp || !SplineComp->GetWorld() || !SplineComp->MetadataClass)
		{
			continue;
		}

		if (!SplineComp->MetadataClass->ClassGeneratedBy)
		{
			continue;
		}

		SplineComp->RefreshMetadata();
	}
}

void FMetaSplineEditorModule::StartupModule()
{
	checkf(GEditor, TEXT("The MetaSplineEditor module needs to be loaded in the PostEngineInit phase."));
	OnBlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddStatic(&FMetaSplineEditorModule::HandleBlueprintPreCompile);
	OnBlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddStatic(&FMetaSplineEditorModule::HandleBlueprintCompiled);
}

void FMetaSplineEditorModule::ShutdownModule()
{
	// During normal shutdown the engine has already been destroyed, so the delegate has been removed without
	// any intevention from us.
	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().Remove(OnBlueprintPreCompileHandle);
		GEditor->OnBlueprintCompiled().Remove(OnBlueprintCompiledHandle);
	}
}

IMPLEMENT_MODULE(FMetaSplineEditorModule, MetaSplineEditor)