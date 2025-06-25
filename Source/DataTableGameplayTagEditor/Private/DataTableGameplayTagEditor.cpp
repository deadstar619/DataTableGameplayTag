// Copyright 2025 Marco Santini. All rights reserved.

#include "DataTableGameplayTagEditor.h"
#include "STagGenWidget.h"

#define LOCTEXT_NAMESPACE "FDataTableGameplayTagEditorModule"

void FDataTableGameplayTagEditorModule::StartupModule()
{
	// Add menu entry Window ▸ Gameplay Tags Generator
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDataTableGameplayTagEditorModule::RegisterMenus));

	// Register the tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
	"GameplayTagsGenerator",
	FOnSpawnTab::CreateLambda(
		[](const FSpawnTabArgs& /*SpawnTabArgs*/) -> TSharedRef<SDockTab>
		{
			return SNew(SDockTab)
				   .TabRole(ETabRole::NomadTab)
				   [
					   SNew(STagGenWidget)
				   ];
		}))
	.SetDisplayName(LOCTEXT("TagGenTabTitle", "Gameplay Tags Generator"))
	.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FDataTableGameplayTagEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("GameplayTagsGenerator");
}

void FDataTableGameplayTagEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped Owner("GameplayTagGenerator");
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Tools");
	Section.AddMenuEntry(
		"OpenTagGen",
		LOCTEXT("OpenTagGen", "Gameplay Tags Generator"),
		LOCTEXT("OpenTagGenTT", "Open the gameplay-tag code generator."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Convert"),
		FUIAction(FExecuteAction::CreateLambda([] {
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId("GameplayTagsGenerator"));
		})));
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FDataTableGameplayTagEditorModule, DataTableGameplayTagEditor)