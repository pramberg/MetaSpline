// Copyright(c) 2021 Viktor Pramberg
#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMetaSplineEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// Updates unmodified point values to potentially new default values.
	static void HandleBlueprintPreCompile(UBlueprint* InBlueprint);

	// Adds or removes curves, based on the variables of the compiled blueprints. 
	static void HandleBlueprintCompiled();

	FDelegateHandle OnBlueprintPreCompileHandle;
	FDelegateHandle OnBlueprintCompiledHandle;
};