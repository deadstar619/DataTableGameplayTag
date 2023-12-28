using UnrealBuildTool;

public class DataTableGameplayTagNodes : ModuleRules
{
    public DataTableGameplayTagNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
                "BlueprintGraph",
                "KismetCompiler",
                "GameplayTags",
                "DataTableGameplayTag",
            }
        );
    }
}