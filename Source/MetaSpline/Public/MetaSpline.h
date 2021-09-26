// Copyright(c) 2021 Viktor Pramberg
#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

METASPLINE_API DECLARE_LOG_CATEGORY_EXTERN(LogMetaSpline, Log, All);

class FMetaSplineModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TUniquePtr<class FMetaSplineDebugRenderer> DebugRenderer;
};
