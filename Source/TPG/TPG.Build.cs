// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPG : ModuleRules
{
	public TPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TPG",
			"TPG/Variant_Platforming",
			"TPG/Variant_Platforming/Animation",
			"TPG/Variant_Combat",
			"TPG/Variant_Combat/AI",
			"TPG/Variant_Combat/Animation",
			"TPG/Variant_Combat/Gameplay",
			"TPG/Variant_Combat/Interfaces",
			"TPG/Variant_Combat/UI",
			"TPG/Variant_SideScrolling",
			"TPG/Variant_SideScrolling/AI",
			"TPG/Variant_SideScrolling/Gameplay",
			"TPG/Variant_SideScrolling/Interfaces",
			"TPG/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
