using UnrealBuildTool;

public class DataTableGameplayTagEditor : ModuleRules
{
    public DataTableGameplayTagEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "Slate",
                "SlateCore",
                "ToolMenus",
            }
        );
    }
}