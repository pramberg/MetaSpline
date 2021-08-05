// Copyright(c) 2021 Viktor Pramberg
using UnrealBuildTool;
using System.IO;

public class MetaSplineEditor : ModuleRules
{
	public MetaSplineEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[]
            {
				Path.Combine(ModuleDirectory, "..", "MetaSpline", "Private"),
            }
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"DetailCustomizations",
				"UnrealEd",
				"EditorStyle",
				"UMG",
				"UMGEditor",
				"PropertyEditor",
				"MetaSpline",
			}
		);
	}
}
