// Copyright(c) 2021 Viktor Pramberg
#include "MetaSpline.h"
#include "MetaSplineDebugRenderer.h"

DEFINE_LOG_CATEGORY(LogMetaSpline);

void FMetaSplineModule::StartupModule()
{
	DebugRenderer.Reset(new FMetaSplineDebugRenderer());
}

void FMetaSplineModule::ShutdownModule()
{
	DebugRenderer.Reset();
}

IMPLEMENT_MODULE(FMetaSplineModule, MetaSpline)