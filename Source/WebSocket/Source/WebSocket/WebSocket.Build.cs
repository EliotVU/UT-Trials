// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class WebSocket : ModuleRules
{
    public WebSocket(TargetInfo Target)
    {
		
		PublicIncludePaths.AddRange(
			new string[] {
				"WebSocket/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "WebSocket/Private",
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Json",
                "JsonUtilities",
			    "libWebSockets",
			    "zlib"
            }
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "Json",
                "JsonUtilities",
			    "libWebSockets",
			    "zlib"
			}
		);
    }
}
