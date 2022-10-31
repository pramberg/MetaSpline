// Copyright(c) 2021 Viktor Pramberg
#pragma once
#include "CoreMinimal.h"
#include "Templates/UnrealTypeTraits.h"

#define IMPL_EXECUTE_ON_PROPERTY_FOR_TYPE(InType) \
static const FName InType##Name(#InType);         \
if (Type == InType##Name)						  \
	return T<InType>::Execute(InArgs...)

#define META_SPLINE_SUPPORTED_TYPES(InExec) \
InExec(double); \
InExec(FVector)

class FMetaSplineTemplateHelpers
{
public:

	/**
 	  * Executes the appropriate templated function depending on the current property type.
	  * Functor should expose a static member function taking any number of variables, with the name 'Execute'
	  * 
	  * #Note: Maybe this should return void, and instead let functions use out parameters if they need output?
	  */
	template<template<typename> typename T, typename... FArgs>
	static auto ExecuteOnProperty(const FProperty* InProperty, FArgs&&... InArgs)
	{
		const FName Type = FName(InProperty->GetCPPType());
		META_SPLINE_SUPPORTED_TYPES(IMPL_EXECUTE_ON_PROPERTY_FOR_TYPE);

		return TInvokeResult_T<decltype(&T<void>::Execute), FArgs...>();
	}
};

#undef IMPL_EXECUTE_ON_PROPERTY_FOR_TYPE