// Copyright(c) 2021 Viktor Pramberg
#pragma once
#include "CoreMinimal.h"

class FMetaSplineTemplateHelpers
{
public:

	/**
 	  * Executes the appropriate templated function depending on the current property type.
	  * Functor should expose a static member function taking any number of variables, with the name 'Execute'
	  */
	template<template<typename> typename T, typename... FArgs>
	static auto ExecuteOnProperty(FProperty* InProperty, FArgs&&... InArgs)
	{
		const FName Type = FName(InProperty->GetCPPType());
		if (Type == TEXT("float"))
			return T<float>::Execute(InArgs...);
		else if (Type == TEXT("FVector"))
			return T<FVector>::Execute(InArgs...);
	}
};
