// Copyright 2023 Marco Santini. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "DataTableGameplayTagFunctionLibrary.generated.h"

class UDataTable;

UCLASS()
class DATATABLEGAMEPLAYTAG_API UDataTableGameplayTagFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	/** Returns the rows of a data table as tags. */
	UFUNCTION(BlueprintCallable, Category = "DataTable")
	static void GetDataTableRowTags(UDataTable* Table, TArray<FGameplayTag>& OutRowTags);
	
	/** Get a Row from a DataTable given a RowName */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "DataTable", meta=(CustomStructureParam = "OutRow", BlueprintInternalUseOnly="true"))
	static bool GetDataTableRowByTag(UDataTable* Table, FGameplayTag Tag, FTableRowBase& OutRow);

	static bool Generic_GetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr);
	
	/** Based on UDataTableFunctionLibrary::GetDataTableRowFromName */
	DECLARE_FUNCTION(execGetDataTableRowByTag)
	{
		P_GET_OBJECT(UDataTable, Table);
		P_GET_STRUCT(FGameplayTag, Tag);
	        
		Stack.StepCompiledIn<FStructProperty>(nullptr);
		void* OutRowPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		bool bSuccess = false;
		
		FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
		if (!Table)
		{
			FBlueprintExceptionInfo ExceptionInfo(
				EBlueprintExceptionType::AccessViolation,
				NSLOCTEXT("GetDataTableRowByTag", "MissingTableInput", "Failed to resolve the table input. Be sure the DataTable is valid.")
			);
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
		else if(StructProp && OutRowPtr)
		{
			UScriptStruct* OutputType = StructProp->Struct;
			const UScriptStruct* TableType  = Table->GetRowStruct();
		
			const bool bCompatible = (OutputType == TableType) || 
				(OutputType->IsChildOf(TableType) && FStructUtils::TheSameLayout(OutputType, TableType));
			if (bCompatible)
			{
				if(Tag.IsValid())
				{
					P_NATIVE_BEGIN;
					bSuccess = Generic_GetDataTableRowFromName(Table, Tag.GetTagName(), OutRowPtr);
					P_NATIVE_END;
				}
				else
				{
					FBlueprintExceptionInfo ExceptionInfo(
						EBlueprintExceptionType::AccessViolation,
						NSLOCTEXT("GetDataTableRowByTag", "MissingTableInput", "Failed to resolve the Tag input. Be sure the Tag is valid.")
					);
					FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
				}
			}
			else
			{
				FBlueprintExceptionInfo ExceptionInfo(
					EBlueprintExceptionType::AccessViolation,
					NSLOCTEXT("GetDataTableRowByTag", "IncompatibleProperty", "Incompatible output parameter; the data table's type is not the same as the return type.")
					);
				FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
			}
		}
		else
		{
			FBlueprintExceptionInfo ExceptionInfo(
				EBlueprintExceptionType::AccessViolation,
				NSLOCTEXT("GetDataTableRowByTag", "MissingOutputProperty", "Failed to resolve the output parameter for GetDataTableRow.")
			);
			FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
		*(bool*)RESULT_PARAM = bSuccess;
	}
};
