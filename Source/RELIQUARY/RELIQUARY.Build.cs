// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RELIQUARY : ModuleRules
{
	public RELIQUARY(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RELIQUARY",
			"RELIQUARY/Variant_Platforming",
			"RELIQUARY/Variant_Platforming/Animation",
			"RELIQUARY/Variant_Combat",
			"RELIQUARY/Variant_Combat/AI",
			"RELIQUARY/Variant_Combat/Animation",
			"RELIQUARY/Variant_Combat/Gameplay",
			"RELIQUARY/Variant_Combat/Interfaces",
			"RELIQUARY/Variant_Combat/UI",
			"RELIQUARY/Variant_SideScrolling",
			"RELIQUARY/Variant_SideScrolling/AI",
			"RELIQUARY/Variant_SideScrolling/Gameplay",
			"RELIQUARY/Variant_SideScrolling/Interfaces",
			"RELIQUARY/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
