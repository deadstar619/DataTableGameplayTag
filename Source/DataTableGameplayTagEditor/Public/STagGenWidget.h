// Copyright 2025 Marco Santini. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Engine/DataTable.h"

class STagGenWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STagGenWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&);

private:
	// UI callbacks
	void OnGenerateClicked();
	TSharedRef<SWidget> MakeDataTablePicker();

	// Input fields
	TSoftObjectPtr<UDataTable> SourceTable;
	FString NamespaceName = TEXT("Skill");
	FString FileName      = TEXT("SkillGameplayTags");
	FString Destination   = TEXT("../GameplayTags/");

	// Helpers
	static bool WriteFiles(UDataTable* Table, const FString& Namespace,
						   const FString& FileStem, const FString& OutDir);
	static FString SafeName  (const FName& Tag);
	static FString BuildHeader(const TArray<struct FGameplayTagTableRow*>& Rows, const FString& Namespace);
	static FString BuildSource(const TArray<struct FGameplayTagTableRow*>& Rows, const FString& Namespace);
};
