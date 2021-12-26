// Copyright(c) 2021 Viktor Pramberg
#include "MetaSpline.h"
#include "MetaSplineDebugRenderer.h"

DEFINE_LOG_CATEGORY(LogMetaSpline);

void FMetaSplineModule::StartupModule()
{
#if !UE_BUILD_SHIPPING
	DebugRenderer.Reset(new FMetaSplineDebugRenderer());
#endif
}

void FMetaSplineModule::ShutdownModule()
{
#if !UE_BUILD_SHIPPING
	DebugRenderer.Reset();
#endif
}

IMPLEMENT_MODULE(FMetaSplineModule, MetaSpline)