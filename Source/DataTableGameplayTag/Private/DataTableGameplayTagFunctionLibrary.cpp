// Copyright 2023 Marco Santini. All rights reserved.

#include "DataTableGameplayTagFunctionLibrary.h"
#include "DataTableGameplayTag.h"
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "DataTableGameplayTagFunctionLibrary"

void UDataTableGameplayTagFunctionLibrary::GetDataTableRowTags(UDataTable* Table, TArray<FGameplayTag>& OutRowTags)
{
	OutRowTags.Empty();
	
	if (Table)
	{
		TArray<FName> RowNames = Table->GetRowNames();
		
		for (const FName& RowName : RowNames)
		{
			const FGameplayTag RowAsTag = FGameplayTag::RequestGameplayTag(RowName, false);
			if(RowAsTag.IsValid() && UGameplayTagsManager::Get().FindTagNode(RowAsTag))
			{
				OutRowTags.Emplace(RowAsTag);
			}
			else
			{
				UE_LOG(LogDataTableGameplayTag, Warning, TEXT("RowName %s is not a valid GameplayTag"), *RowName.ToString());
			}
		}
	}
}

bool UDataTableGameplayTagFunctionLibrary::GetDataTableRowByTag(UDataTable* Table, FGameplayTag Tag, FTableRowBase& OutRow)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(0);
	return false;
}


bool UDataTableGameplayTagFunctionLibrary::Generic_GetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr)
{
	bool bFoundRow = false;

	if (OutRowPtr && Table)
	{
		const void* RowPtr = Table->FindRowUnchecked(RowName);

		if (RowPtr != nullptr)
		{
			const UScriptStruct* StructType = Table->GetRowStruct();

			if (StructType != nullptr)
			{
				StructType->CopyScriptStruct(OutRowPtr, RowPtr);
				bFoundRow = true;
			}
		}
	}

	return bFoundRow;
}

#undef LOCTEXT_NAMESPACE
