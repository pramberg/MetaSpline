// Copyright(c) 2021 Viktor Pramberg
#include "MetaSplineSettings.h"

#define LOCTEXT_NAMESPACE "MetaSplineSettings"

// Project settings
UMetaSplineSettings::UMetaSplineSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("MetaSpline");
}

#if WITH_EDITOR
FText UMetaSplineSettings::GetSectionText() const
{
	return LOCTEXT("UserSettingsDisplayName", "MetaSpline");
}

#endif	// WITH_EDITOR

// User settings
UMetaSplineUserSettings::UMetaSplineUserSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("MetaSpline");
}

#if WITH_EDITOR
FText UMetaSplineUserSettings::GetSectionText() const
{
	return LOCTEXT("UserSettingsDisplayName", "MetaSpline");
}

#endif	// WITH_EDITOR

#undef LOCTEXT_NAMESPACE