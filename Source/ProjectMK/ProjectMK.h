// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FProjectMKModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	void RegisterEditorDataStorageFilter();
	FDelegateHandle DataStorageEnabledHandle;
	bool bRegisteredDataStorageFilter = false;
#endif
};
