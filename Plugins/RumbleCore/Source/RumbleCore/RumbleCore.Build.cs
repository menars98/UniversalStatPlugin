// Copyright (c) 2026 Menars. All Rights Reserved.
// Universal Stat & Effect Framework

using UnrealBuildTool;

public class RumbleCore : ModuleRules
{
	public RumbleCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Niagara"
			}
		);
	}
}