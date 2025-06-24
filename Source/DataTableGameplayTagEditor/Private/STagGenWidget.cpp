#include "STagGenWidget.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "GameplayTagsManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "GameplayTagGen"

/***************************  Construct  **************************************/
void STagGenWidget::Construct(const FArguments&)
{
	// Collect all C++ modules of this project (runtime & editor)
	for (const FModuleContextInfo& M : GameProjectUtils::GetCurrentProjectModules())
	{
		Modules.Emplace(MakeShared<FModuleContextInfo>(M));
	}
	ensure(Modules.Num() > 0);
	SelectedModule = Modules[0];

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				// ───── DataTable picker ─────
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock).Text(LOCTEXT("DTLabel", "Source DataTable"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					MakeDataTablePicker()
				]

				// ───── Namespace ─────
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock).Text(LOCTEXT("NSLabel", "Namespace"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]{ return FText::FromString(NamespaceName); })
					.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type){ NamespaceName = T.ToString().TrimStartAndEnd(); })
				]

				// ───── File stem ─────
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock).Text(LOCTEXT("FileStemLabel", "File name (no extension)"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]{ return FText::FromString(FileStem); })
					.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type){ FileStem = T.ToString().TrimStartAndEnd(); })
				]

				// ───── Destination (module + relative dir) ─────
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 8, 2, 2)
				[
					SNew(STextBlock).Text(LOCTEXT("DestLabel", "Destination"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)

					// module combo
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)
					[
						MakeModuleCombo()
					]
					// relative path edit
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SAssignNew(RelPathEdit, SEditableTextBox)
						.Text_Lambda([this]{ return FText::FromString(RelPath); })
						.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type){ RelPath = T.ToString().TrimStartAndEnd(); })
					]
				]

				// ───── Generate button ─────
				+ SVerticalBox::Slot().AutoHeight().Padding(4,10)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.Text(LOCTEXT("Generate", "Generate"))
					.OnClicked(this, &STagGenWidget::OnGenerateClicked)
				]
			]
		]
	];
}

/*************************  Asset picker  *************************************/
TSharedRef<SWidget> STagGenWidget::MakeDataTablePicker()
{
	FAssetPickerConfig Picker;
	Picker.Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Picker.SelectionMode           = ESelectionMode::Single;
	Picker.bAllowNullSelection     = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& Asset)
	{
		SourceTable = Cast<UDataTable>(Asset.GetAsset());
	});

	return FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().CreateAssetPicker(Picker);
}

/*************************  Module combo  *************************************/
TSharedRef<SWidget> STagGenWidget::MakeModuleCombo()
{
	return SNew(SComboBox<TSharedPtr<FModuleContextInfo>>)
		.OptionsSource(&Modules)
		.InitiallySelectedItem(SelectedModule)
		.OnSelectionChanged_Lambda([this](TSharedPtr<FModuleContextInfo> NewSel, ESelectInfo::Type)
		{
			SelectedModule = NewSel;
		})
		.OnGenerateWidget_Lambda([](TSharedPtr<FModuleContextInfo> M)
		{
			return SNew(STextBlock).Text(FText::FromString(M->ModuleName));
		})
		[
			SNew(STextBlock).Text_Lambda([this]{ return FText::FromString(SelectedModule->ModuleName); })
		];
}

/*************************  Generate click  ***********************************/
FReply STagGenWidget::OnGenerateClicked()
{
	if (!SourceTable.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoTable", "Please select a DataTable."));
		return FReply::Handled();
	}

	if (WriteFiles())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Success", "Gameplay‑tag files generated."));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Fail", "Failed to write files. Check the log for details."));
	}
	return FReply::Handled();
}

/*************************  Helper utils  *************************************/
FString STagGenWidget::SafeName(const FName& Tag)
{
	FString R = Tag.ToString();
	R.ReplaceInline(TEXT("."), TEXT("_"));
	return R;
}

FString STagGenWidget::BuildHeader(const TArray<FGameplayTagTableRow*>& Rows, const FString& NS)
{
	static constexpr TCHAR HeaderTpl[] =
		TEXT("#pragma once\n\n#include \"NativeGameplayTags.h\"\n\nnamespace %s\n{\n");
	FString Out = FString::Printf(HeaderTpl, *NS);

	for (const FGameplayTagTableRow* Row : Rows)
	{
		const FString Name = SafeName(Row->Tag);
		Out += FString::Printf(TEXT("\t// %s\n\tUE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_%s_%s);\n\n"),
			*Row->DevComment, *NS, *Name);
	}
	Out.Append(TEXT("}\n"));
	return Out;
}

FString STagGenWidget::BuildSource(const TArray<FGameplayTagTableRow*>& Rows,
                                   const FString& NS,
                                   const FString& HeaderStem)
{
	static constexpr TCHAR SrcTpl[] = TEXT("#include \"%s.h\"\n\nnamespace %s\n{\n");
	FString Out = FString::Printf(SrcTpl, *HeaderStem, *NS);

	for (const FGameplayTagTableRow* Row : Rows)
	{
		const FString Name = SafeName(Row->Tag);
		Out += FString::Printf(TEXT("\t// %s\n\tUE_DEFINE_GAMEPLAY_TAG(TAG_%s_%s, \"%s\");\n\n"),
			*Row->DevComment, *NS, *Name, *Row->Tag.ToString());
	}
	Out.Append(TEXT("}\n"));
	return Out;
}

/*************************  Write files  **************************************/
bool STagGenWidget::WriteFiles()
{
	check(SelectedModule.IsValid());

	// Resolve destination directory:  <Module>/Source/<RelPath>/
	FString OutDir = SelectedModule->ModuleSourcePath / RelPath;
	FPaths::NormalizeDirectoryName(OutDir);
	if (!OutDir.EndsWith(TEXT("/"))) OutDir.AppendChar('/');

	// Create directory tree if needed
	IFileManager& FM = IFileManager::Get();
	if (!FM.MakeDirectory(*OutDir, /*Tree*/true))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot create directory %s"), *OutDir);
		return false;
	}

	// Gather rows
	TArray<FGameplayTagTableRow*> Rows;
	SourceTable->GetAllRows(TEXT("TagGenWidget"), Rows);

	const FString HeaderPath = OutDir / (FileStem + TEXT(".h"));
	const FString SourcePath = OutDir / (FileStem + TEXT(".cpp"));

	bool bOk = FFileHelper::SaveStringToFile(BuildHeader(Rows, NamespaceName), *HeaderPath);
	bOk &= FFileHelper::SaveStringToFile(BuildSource(Rows, NamespaceName, FileStem), *SourcePath);

	if (!bOk)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write header or source file for gameplay tags"));
	}
	return bOk;
}

#undef LOCTEXT_NAMESPACE