// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "IROSBridgePlugin.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FROSBridgePlugin : public IROSBridgePlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FROSBridgePlugin, ROSBridgePlugin )



void FROSBridgePlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FROSBridgePlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}


