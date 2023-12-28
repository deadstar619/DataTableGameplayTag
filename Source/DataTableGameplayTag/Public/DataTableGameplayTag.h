// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DATATABLEGAMEPLAYTAG_API DECLARE_LOG_CATEGORY_EXTERN(LogDataTableGameplayTag, Log, All);

class FDataTableGameplayTagModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
