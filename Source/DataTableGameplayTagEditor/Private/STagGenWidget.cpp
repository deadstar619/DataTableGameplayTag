// Fill out your copyright notice in the Description page of Project Settings.

#include "STagGenWidget.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "GameplayTagsManager.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "GameplayTagGen"

void STagGenWidget::Construct(const FArguments&)
{
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)

			// DataTable picker
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("DTLabel", "Source DataTable"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				MakeDataTablePicker()
			]

			// Namespace
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("NamespaceLabel", "Namespace"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this]{ return FText::FromString(NamespaceName); })
				.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type) { NamespaceName = T.ToString(); })
			]

			// Filename stem
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("FileLabel", "File name (no extension)"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this]{ return FText::FromString(FileName); })
				.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type) { FileName = T.ToString(); })
			]

			// Destination path
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(STextBlock).Text(LOCTEXT("DestLabel", "Destination (relative/absolute)"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(SEditableTextBox)
				.Text_Lambda([this]{ return FText::FromString(Destination); })
				.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type) { Destination = T.ToString(); })
			]

			// Generate button
			+ SVerticalBox::Slot().AutoHeight().Padding(10)
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.Text(LOCTEXT("Generate", "Generate"))
				.OnClicked_Lambda([this](){ OnGenerateClicked(); return FReply::Handled(); })
			]
		]
	];
}

/* ---------------------------  UI HELPERS  --------------------------- */

TSharedRef<SWidget> STagGenWidget::MakeDataTablePicker()
{
	FAssetPickerConfig Picker;
	Picker.Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Picker.SelectionMode = ESelectionMode::Single;
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& Asset){
		SourceTable = Cast<UDataTable>(Asset.GetAsset());
	});
	return FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().
		CreateAssetPicker(Picker);
}

void STagGenWidget::OnGenerateClicked()
{
	if (!SourceTable.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoTable", "Please select a DataTable."));
		return;
	}

	if (WriteFiles(SourceTable.Get(), NamespaceName, FileName, Destination))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Success", "Gameplay-tag files generated."));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Fail", "Failed to write files."));
	}
}

/* --------------------------  GENERATION  ---------------------------- */

static const FString HeaderTemplatePreamble =
	TEXT("#pragma once\n\n#include \"NativeGameplayTags.h\"\n\nnamespace %s\n{\n");
static const FString SourceTemplatePreamble =
	TEXT("#include \"%s.h\"\n\nnamespace %s\n{\n");

FString STagGenWidget::SafeName(const FName& Tag)
{
	FString R = Tag.ToString();
	R.ReplaceInline(TEXT("."), TEXT("_"));
	return R;
}

FString STagGenWidget::BuildHeader(const TArray<FGameplayTagTableRow*>& Rows, const FString& NS)
{
	FString Out = FString::Printf(
		TEXT("#pragma once\n\n#include \"NativeGameplayTags.h\"\n\nnamespace %s\n{\n"),
		*NS);

	for (const auto* Row : Rows)
	{
		const FString Name = SafeName(Row->Tag);
		Out += FString::Printf(
			TEXT("\t// %s\n\tUE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_%s_%s);\n\n"),
			*Row->DevComment, *NS, *Name);
	}
	Out += TEXT("}\n");
	return Out;
}

FString STagGenWidget::BuildSource(const TArray<FGameplayTagTableRow*>& Rows, const FString& NS)
{
	FString Out = FString::Printf(
		TEXT("#include \"%s.h\"\n\nnamespace %s\n{\n"),
		*NS,   // header file stem
		*NS);  // namespace

	for (const auto* Row : Rows)
	{
		const FString Name = SafeName(Row->Tag);
		Out += FString::Printf(
			TEXT("\t// %s\n\tUE_DEFINE_GAMEPLAY_TAG(TAG_%s_%s, \"%s\");\n\n"),
			*Row->DevComment, *NS, *Name, *Row->Tag.ToString());
	}
	Out += TEXT("}\n");
	return Out;
}

bool STagGenWidget::WriteFiles(UDataTable* Table, const FString& NS,
                               const FString& FileStem, const FString& OutDir)
{
	TArray<FGameplayTagTableRow*> Rows;
	Table->GetAllRows(TEXT("TagGenWidget"), Rows);

	const FString HeaderPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(OutDir, FileStem + TEXT(".h")));
	const FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(OutDir, FileStem + TEXT(".cpp")));

	return	FFileHelper::SaveStringToFile(BuildHeader(Rows, NS), *HeaderPath) &&
			FFileHelper::SaveStringToFile(BuildSource(Rows, NS), *SourcePath);
}

#undef LOCTEXT_NAMESPACE
