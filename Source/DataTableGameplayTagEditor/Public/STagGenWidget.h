// Copyright 2025 Marco Santini. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Engine/DataTable.h"
#include "GameProjectUtils.h"
#include "Templates/SharedPointer.h"
#include "Input/Reply.h"

class SEditableTextBox;
struct FGameplayTagTableRow;
struct FModuleContextInfo;

class STagGenWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STagGenWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/*********************  UI callbacks  *********************/
	FReply OnGenerateClicked();
	TSharedRef<SWidget> MakeDataTablePicker();
	TSharedRef<SWidget> MakeModuleCombo();

	/*********************  Inputs  ***************************/
	TSoftObjectPtr<UDataTable>            SourceTable;
	FString                               NamespaceName = TEXT("Skill");
	FString                               FileStem      = TEXT("SkillGameplayTags");

	//  Module + relative directory
	TArray<TSharedPtr<FModuleContextInfo>> Modules;
	TSharedPtr<FModuleContextInfo>         SelectedModule;
	FString                               RelPath = TEXT("GameplayTags");
	TSharedPtr<SEditableTextBox>          RelPathEdit;

	/*********************  Helpers  **************************/
	bool WriteFiles();
	static FString SafeName(const FName& Tag);
	static FString BuildHeader(const TArray<FGameplayTagTableRow*>& Rows,
							   const FString& NS);
	static FString BuildSource(const TArray<FGameplayTagTableRow*>& Rows,
							   const FString& NS,
							   const FString& HeaderStem);
};
