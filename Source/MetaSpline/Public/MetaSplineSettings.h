// Copyright(c) 2021 Viktor Pramberg
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetaSplineSettings.generated.h"


UCLASS(config = Game, defaultconfig)
class METASPLINE_API UMetaSplineSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMetaSplineSettings();

#if WITH_EDITOR
	//~ UDeveloperSettings interface
	virtual FText GetSectionText() const override;
#endif
};

UCLASS(config = EditorPerProjectUserSettings, defaultconfig)
class METASPLINE_API UMetaSplineUserSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UMetaSplineUserSettings();

#if WITH_EDITOR
	//~ UDeveloperSettings interface
	virtual FText GetSectionText() const override;
#endif

	/** The number of points to draw. -1 means draw all points. */
	UPROPERTY(config, EditAnywhere, Category = "Debug")
	int32 NumberOfPoints = -1;
};
