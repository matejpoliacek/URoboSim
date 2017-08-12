// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealRobotsP3 : ModuleRules
{
	public UnrealRobotsP3(TargetInfo Target)
	{

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "JsonUtilities", "InputCore", "HeadMountedDisplay", "XmlParser", "UnrealEd", "UnrealRobots", "ROSBridgePlugin" });
		
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealRobots", "ROSBridgePlugin" });
	}
}
