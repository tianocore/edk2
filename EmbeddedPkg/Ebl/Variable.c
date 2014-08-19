/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "Ebl.h"

#include <Guid/GlobalVariable.h>

EFI_STATUS
EblGetCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS        Status = EFI_INVALID_PARAMETER;
  UINTN       Size;
  VOID*       Value;
  CHAR8*      AsciiVariableName = NULL;
  CHAR16*     VariableName;
  UINT32      Index;

  if (Argc == 1) {
    AsciiPrint("Variable name is missing.\n");
    return Status;
  }

  for (Index = 1; Index < Argc; Index++) {
    if (Argv[Index][0] == '-') {
      AsciiPrint("Warning: '%a' not recognized.\n",Argv[Index]);
    } else {
      AsciiVariableName = Argv[Index];
    }
  }

  if (AsciiVariableName == NULL) {
    AsciiPrint("Variable name is missing.\n");
    return Status;
  } else {
    VariableName = AllocatePool((AsciiStrLen (AsciiVariableName) + 1) * sizeof (CHAR16));
    AsciiStrToUnicodeStr (AsciiVariableName,VariableName);
  }

  // Try to get the variable size.
  Value = NULL;
  Size = 0;
  Status = gRT->GetVariable (VariableName, &gEfiGlobalVariableGuid, NULL, &Size, Value);
  if (Status == EFI_NOT_FOUND) {
    AsciiPrint("Variable name '%s' not found.\n",VariableName);
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    // Get the environment variable value
    Value = AllocatePool (Size);
    if (Value == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable ((CHAR16 *)VariableName, &gEfiGlobalVariableGuid, NULL, &Size, Value);
    if (EFI_ERROR (Status)) {
      AsciiPrint("Error: '%r'\n",Status);
    } else {
      AsciiPrint("%a=%a\n",AsciiVariableName,Value);
    }
    FreePool(Value);
  } else {
    AsciiPrint("Error: '%r'\n",Status);
  }

  FreePool(VariableName);
  return Status;
}

EFI_STATUS
EblSetCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS    Status = EFI_INVALID_PARAMETER;
  CHAR8*        AsciiVariableSetting = NULL;
  CHAR8*        AsciiVariableName;
  CHAR8*        AsciiValue;
  UINT32        AsciiValueLength;
  CHAR16*       VariableName;
  UINT32        Index;
  UINT32        EscapedQuotes = 0;
  BOOLEAN       Volatile = FALSE;

  if (Argc == 1) {
    AsciiPrint("Variable name is missing.\n");
    return Status;
  }

  for (Index = 1; Index < Argc; Index++) {
    if (AsciiStrCmp(Argv[Index],"-v") == 0) {
      Volatile = 0;
    } else if (Argv[Index][0] == '-') {
      AsciiPrint("Warning: '%a' not recognized.\n",Argv[Index]);
    } else {
      AsciiVariableSetting = Argv[Index];
    }
  }

  if (AsciiVariableSetting == NULL) {
    AsciiPrint("Variable name is missing.\n");
    return Status;
  }

  // Check if it is a valid variable setting
  AsciiValue = AsciiStrStr (AsciiVariableSetting,"=");
  if (AsciiValue == NULL) {
    //
    // There is no value. It means this variable will be deleted
    //

    // Convert VariableName into Unicode
    VariableName = AllocatePool((AsciiStrLen (AsciiVariableSetting) + 1) * sizeof (CHAR16));
    AsciiStrToUnicodeStr (AsciiVariableSetting,VariableName);

    Status = gRT->SetVariable (
                          VariableName,
                          &gEfiGlobalVariableGuid,
                          ( !Volatile ? EFI_VARIABLE_NON_VOLATILE : 0) |
                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                          0,
                          NULL
                          );
    if (!EFI_ERROR(Status)) {
      AsciiPrint("Variable '%s' deleted\n",VariableName);
    } else {
      AsciiPrint("Variable setting is incorrect. It should be VariableName=Value\n");
    }
    return Status;
  }

  AsciiValue[0] = '\0';
  AsciiVariableName = AsciiVariableSetting;
  AsciiValue++;

  // Clean AsciiValue from quote
  if (AsciiValue[0] == '"') {
    AsciiValue++;
  }
  AsciiValueLength = AsciiStrLen (AsciiValue);
  if ((AsciiValue[AsciiValueLength-2] != '\\') && (AsciiValue[AsciiValueLength-1] == '"')) {
    AsciiValue[AsciiValueLength-1] = '\0';
  }

  // Clean AsciiValue from escaped quotes
  for (Index = 0; Index < AsciiValueLength; Index++) {
    if ((Index > 0) && (AsciiValue[Index-1] == '\\') && (AsciiValue[Index] == '"')) {
      EscapedQuotes++;
    }
    AsciiValue[Index-EscapedQuotes] = AsciiValue[Index];
  }
  // Fill the end of the value with '\0'
  for (Index = 0; Index < EscapedQuotes; Index++) {
    AsciiValue[AsciiValueLength-1-Index] = '\0';
  }

  // Convert VariableName into Unicode
  VariableName = AllocatePool((AsciiStrLen (AsciiVariableName) + 1) * sizeof (CHAR16));
  AsciiStrToUnicodeStr (AsciiVariableName,VariableName);

  Status = gRT->SetVariable (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      ( !Volatile ? EFI_VARIABLE_NON_VOLATILE : 0) |
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrLen (AsciiValue)+1,
                      AsciiValue
                      );
  if (!EFI_ERROR(Status)) {
    AsciiPrint("'%a'='%a'\n",AsciiVariableName,AsciiValue);
  }

  return Status;
}

GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdVariableTemplate[] =
{
  {
    "get",
    " ; get UEFI variable\n\r   [v]; verbose",
    NULL,
    EblGetCmd
  },
  {
    "set",
    " ; set UEFI variable\n\r   [v]; create volatile variable",
    NULL,
    EblSetCmd
  }
};

/**
  Initialize the commands in this in this file
**/
VOID
EblInitializeVariableCmds (
  VOID
  )
{
  EblAddCommands (mCmdVariableTemplate, sizeof (mCmdVariableTemplate)/sizeof (EBL_COMMAND_TABLE));
}
