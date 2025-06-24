#include "STagGenWidget.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "GameplayTagsManager.h"
#include "IContentBrowserSingleton.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "DesktopPlatformModule.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "GameplayTagGen"

/***************************  Construct  **************************************/
void STagGenWidget::Construct(const FArguments&)
{
    // Collect all C++ modules of this project (runtime, editor, plugin)
    for (const FModuleContextInfo& M : GameProjectUtils::GetCurrentProjectModules())
        Modules.Emplace(MakeShared<FModuleContextInfo>(M));
    for (const FModuleContextInfo& M : GameProjectUtils::GetCurrentProjectPluginModules())
        Modules.Emplace(MakeShared<FModuleContextInfo>(M));

    ensure(Modules.Num() > 0);
    SelectedModule = Modules[0];
    RelPath.Empty();

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

                // DataTable picker
                + SVerticalBox::Slot().AutoHeight().Padding(2)
                [
                    SNew(STextBlock).Text(LOCTEXT("DTLabel", "Source DataTable"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(2)
                [
                    MakeDataTablePicker()
                ]

                // Namespace
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

                // File stem
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

                // Destination (module + relative dir)
                + SVerticalBox::Slot().AutoHeight().Padding(2, 8, 2, 2)
                [
                    SNew(STextBlock).Text(LOCTEXT("DestLabel", "Destination Module & Folder (Public)"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(2)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                    [
                        MakeModuleCombo()
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f)
                    [
                        SNew(SEditableTextBox)
                        .Text_Lambda([this]{ return FText::FromString(RelPath); })
                        .IsReadOnly(true)
                        .HintText(LOCTEXT("PublicSubdirHint", "Choose a subfolder (optional)"))
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4,0,0,0)
                    [
                        SNew(SButton)
                        .ToolTipText(LOCTEXT("ChooseFolderTT", "Choose a subfolder within the module's Public folder"))
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .OnClicked(this, &STagGenWidget::OnChooseFolderClicked)
                        [
                            SNew(SImage)
                            .Image(FAppStyle::Get().GetBrush("Icons.FolderClosed"))
                            .ColorAndOpacity(FSlateColor::UseForeground())
                        ]
                    ]
                ]

                // Path preview
                + SVerticalBox::Slot().AutoHeight().Padding(2, 8, 2, 2)
                [
                    SNew(STextBlock)
                    .Font(FAppStyle::GetFontStyle("Monospaced"))
                    .Text(this, &STagGenWidget::GetPathPreviewText)
                ]

                // Generate button
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
            RelPath.Empty();
        })
        .OnGenerateWidget_Lambda([](TSharedPtr<FModuleContextInfo> M)
        {
            return SNew(STextBlock).Text(FText::FromString(M->ModuleName));
        })
        [
            SNew(STextBlock).Text_Lambda([this]{ return FText::FromString(SelectedModule->ModuleName); })
        ];
}

/*************************  Choose folder  ************************************/
FReply STagGenWidget::OnChooseFolderClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform || !SelectedModule.IsValid())
        return FReply::Handled();

    TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
    void* ParentWindowHandle = ParentWindow.IsValid() ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

    FString PublicRoot = SelectedModule->ModuleSourcePath / TEXT("Public");
    FString ChosenFolder;
    const bool bPicked = DesktopPlatform->OpenDirectoryDialog(
        ParentWindowHandle,
        TEXT("Choose Header Folder (Public)"),
        PublicRoot,
        ChosenFolder
    );
    if (bPicked && ChosenFolder.StartsWith(PublicRoot))
    {
        FString Rel = ChosenFolder.Mid(PublicRoot.Len());
        Rel.RemoveFromStart(TEXT("/"));
        Rel.RemoveFromEnd(TEXT("/"));
        RelPath = Rel;
    }
    return FReply::Handled();
}

/*************************  Path preview  **************************************/
FText STagGenWidget::GetPathPreviewText() const
{
    FString HeaderPath, SourcePath;
    ComputeOutputPaths(HeaderPath, SourcePath);

    FString Preview = FString::Printf(TEXT("Will generate:\n%s\n%s"), *HeaderPath, *SourcePath);
    return FText::FromString(Preview);
}

/*************************  Generate click  ***********************************/
FReply STagGenWidget::OnGenerateClicked()
{
    if (!SourceTable.IsValid())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoTable", "Please select a DataTable."));
        return FReply::Handled();
    }

    FString HeaderPath, SourcePath;
    ComputeOutputPaths(HeaderPath, SourcePath);

    IFileManager& FM = IFileManager::Get();
    bool bHeaderExists = FM.FileExists(*HeaderPath);
    bool bSourceExists = FM.FileExists(*SourcePath);
    if (bHeaderExists || bSourceExists)
    {
        FText WarnMsg = FText::Format(LOCTEXT("FilesExistWarn",
            "The following files already exist and will be overwritten:\n{0}{1}{2}Continue?"),
            bHeaderExists ? FText::FromString(HeaderPath + TEXT("\n")) : FText::GetEmpty(),
            bSourceExists ? FText::FromString(SourcePath + TEXT("\n")) : FText::GetEmpty(),
            FText::GetEmpty());
        if (FMessageDialog::Open(EAppMsgType::YesNo, WarnMsg) != EAppReturnType::Yes)
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
        Out += FString::Printf(TEXT("\tUE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_%s_%s);\n"),
            *NS, *Name);
    }
    Out.Append(TEXT("}\n"));
    return Out;
}

FString STagGenWidget::BuildSource(const TArray<FGameplayTagTableRow*>& Rows,
                                   const FString& NS,
                                   const FString& HeaderStem)
{
    // Determine the relative path for the include.
    FString IncludePath = HeaderStem + TEXT(".h");
    // If HeaderStem contains directories (eg: "Sub/Dir/SkillGameplayTags"), use as is.

    static constexpr TCHAR SrcTpl[] = TEXT("#include \"%s\"\n\nnamespace %s\n{\n");
    FString Out = FString::Printf(SrcTpl, *IncludePath, *NS);

    for (const FGameplayTagTableRow* Row : Rows)
    {
        const FString Name = SafeName(Row->Tag);
        Out += FString::Printf(TEXT("\tUE_DEFINE_GAMEPLAY_TAG(TAG_%s_%s, \"%s\");\n"),
            *NS, *Name, *Row->Tag.ToString());
    }
    Out.Append(TEXT("}\n"));
    return Out;
}

void STagGenWidget::ComputeOutputPaths(FString& OutHeader, FString& OutSource) const
{
    check(SelectedModule.IsValid());

    FString Root = SelectedModule->ModuleSourcePath;
    FString PublicDir = Root / TEXT("Public");
    FString PrivateDir = Root / TEXT("Private");
    if (!RelPath.IsEmpty())
    {
        PublicDir /= RelPath;
        PrivateDir /= RelPath;
    }
    FPaths::NormalizeDirectoryName(PublicDir);
    FPaths::NormalizeDirectoryName(PrivateDir);
    if (!PublicDir.EndsWith(TEXT("/"))) PublicDir.AppendChar('/');
    if (!PrivateDir.EndsWith(TEXT("/"))) PrivateDir.AppendChar('/');

    OutHeader = PublicDir / (FileStem + TEXT(".h"));
    OutSource = PrivateDir / (FileStem + TEXT(".cpp"));
}

/*************************  Write files  **************************************/
bool STagGenWidget::WriteFiles()
{
    check(SelectedModule.IsValid());

    FString HeaderPath, SourcePath;
    ComputeOutputPaths(HeaderPath, SourcePath);

    // Create directory tree if needed
    FString OutHeaderDir = FPaths::GetPath(HeaderPath);
    FString OutSourceDir = FPaths::GetPath(SourcePath);
    IFileManager& FM = IFileManager::Get();
    if (!FM.MakeDirectory(*OutHeaderDir, /*Tree*/true))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create directory %s"), *OutHeaderDir);
        return false;
    }
    if (!FM.MakeDirectory(*OutSourceDir, /*Tree*/true))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create directory %s"), *OutSourceDir);
        return false;
    }

    // Gather rows
    TArray<FGameplayTagTableRow*> Rows;
    SourceTable->GetAllRows(TEXT("TagGenWidget"), Rows);

    bool bOk = FFileHelper::SaveStringToFile(BuildHeader(Rows, NamespaceName), *HeaderPath);

    // Compute the relative header include
    FString IncludeRel;
    if (!RelPath.IsEmpty())
    {
        IncludeRel = RelPath / (FileStem + TEXT(".h"));  
    }
    else
    {
        IncludeRel = FileStem + TEXT(".h"); 
    }

    IncludeRel.ReplaceInline(TEXT("\\"), TEXT("/"));

    bOk &= FFileHelper::SaveStringToFile(BuildSource(Rows, NamespaceName, IncludeRel), *SourcePath);
    
    if (!bOk)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to write header or source file for gameplay tags"));
    }
    return bOk;
}

#undef LOCTEXT_NAMESPACE
