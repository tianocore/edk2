/** @file
  Main file for DmpStore shell Debug1 function.

  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"


#define INIT_NAME_BUFFER_SIZE  128
#define INIT_DATA_BUFFER_SIZE  1024
#define INIT_ATTS_BUFFER_SIZE  64

CONST CHAR16 *
EFIAPI
GetAttrType (
  IN CONST UINT32 Atts,
  IN OUT   CHAR16 *RetString
  )
{
  StrCpy(RetString, L"");

  if (Atts & EFI_VARIABLE_NON_VOLATILE) {
    StrCat(RetString, L"+NV");
  }
  if (Atts & EFI_VARIABLE_RUNTIME_ACCESS) {
    StrCat(RetString, L"+RS+BS");
  } else if (Atts & EFI_VARIABLE_BOOTSERVICE_ACCESS) {
    StrCat(RetString, L"+BS");
  }
  if (Atts & EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    StrCat(RetString, L"+HR");
  }
  if (Atts & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    StrCat(RetString, L"+AW");
  }
  if (Atts & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
    StrCat(RetString, L"+AT");
  }

  if (RetString[0] == L'+') {
    return (RetString+1);
  }
  if (RetString[0] == CHAR_NULL) {
    StrCpy(RetString, L"invalid");
    return (RetString);
  }
  return (RetString);
}

/**
  Function to display or delete variables.

  @param[in] VariableName   The variable name of the EFI variable (or NULL).
  @param[in] Guid           The GUID of the variable set (or NULL).
  @param[in] Delete         TRUE to delete, FALSE otherwise.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
EFIAPI
ProcessVariables (
  IN CONST CHAR16   *VariableName OPTIONAL,
  IN CONST EFI_GUID *Guid OPTIONAL,
  IN BOOLEAN        Delete
  )
{
  EFI_STATUS                Status;
  CHAR16                    *FoundVarName;
  EFI_GUID                  FoundVarGuid;
  UINT8                     *DataBuffer;
  UINTN                     DataSize;
  UINT32                    Atts;
  SHELL_STATUS              ShellStatus;
  BOOLEAN                   Found;
  UINTN                     NameBufferSize; // Allocated Name buffer size
  UINTN                     NameSize;
  CHAR16                    *OldName;
  UINTN                     OldNameBufferSize;
  UINTN                     DataBufferSize; // Allocated data buffer size
  CHAR16                    RetString[INIT_ATTS_BUFFER_SIZE];

  Found         = FALSE;
  ShellStatus   = SHELL_SUCCESS;
  Status        = EFI_SUCCESS;

  NameBufferSize = INIT_NAME_BUFFER_SIZE;
  DataBufferSize = INIT_DATA_BUFFER_SIZE;
  FoundVarName   = AllocateZeroPool (NameBufferSize);
  if (FoundVarName == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }  
  DataBuffer     = AllocatePool (DataBufferSize);
  if (DataBuffer == NULL) {
    FreePool (FoundVarName);
    return (SHELL_OUT_OF_RESOURCES);
  }

  for (;;){
    if (ShellGetExecutionBreakFlag()) {
      ShellStatus = SHELL_ABORTED;
      break;
    }

    NameSize  = NameBufferSize;
    Status    = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      OldName           = FoundVarName;
      OldNameBufferSize = NameBufferSize;
      //
      // Expand at least twice to avoid reallocate many times
      //
      NameBufferSize = NameSize > NameBufferSize * 2 ? NameSize : NameBufferSize * 2;
      FoundVarName           = AllocateZeroPool (NameBufferSize);
      if (FoundVarName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        FreePool (OldName);
        break;
      }
      //
      // Preserve the original content to get correct iteration for GetNextVariableName() call
      //
      CopyMem (FoundVarName, OldName, OldNameBufferSize);
      FreePool (OldName);
      NameSize = NameBufferSize;
      Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
    }
    if (Status == EFI_NOT_FOUND) {
      break;
    }
    ASSERT_EFI_ERROR(Status);

    //
    // Check if it matches
    //
    if (VariableName != NULL) {
      if (!gUnicodeCollation->MetaiMatch(gUnicodeCollation, FoundVarName, (CHAR16*)VariableName)) {
        continue;
      }
    }
    if (Guid != NULL) {
      if (!CompareGuid(&FoundVarGuid, Guid)) {
        continue;
      }
    }

    DataSize  = DataBufferSize;
    Status    = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // Expand at least twice to avoid reallocate many times
      //
      FreePool (DataBuffer);
      DataBufferSize = DataSize > DataBufferSize * 2 ? DataSize : DataBufferSize * 2;
      DataBuffer           = AllocatePool (DataBufferSize);
      if (DataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      DataSize = DataBufferSize;
      Status   = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
    }    
    ASSERT_EFI_ERROR(Status);

    //
    // do the print or delete
    //
    Found = TRUE;
    if (!Delete) {
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN(STR_DMPSTORE_HEADER_LINE),
        gShellDebug1HiiHandle,
        GetAttrType(Atts, RetString),
        &FoundVarGuid,
        FoundVarName,
        DataSize);
      DumpHex(2, 0, DataSize, DataBuffer);
    } else {
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN(STR_DMPSTORE_DELETE_LINE),
        gShellDebug1HiiHandle,
        &FoundVarGuid,
        FoundVarName);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN(STR_DMPSTORE_DELETE_DONE),
        gShellDebug1HiiHandle,
        gRT->SetVariable(FoundVarName, &FoundVarGuid, Atts, 0, NULL));
        FoundVarName[0] = CHAR_NULL;
    }
  }

  if (FoundVarName != NULL) {
    FreePool(FoundVarName);
  }
  if (DataBuffer != NULL) {
    FreePool(DataBuffer);
  }
  if (!Found) {
    if (Status == EFI_OUT_OF_RESOURCES) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle);
      return SHELL_OUT_OF_RESOURCES;
    }

    if (VariableName != NULL && Guid == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_N), gShellDebug1HiiHandle, VariableName);
    } else if (VariableName != NULL && Guid != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_GN), gShellDebug1HiiHandle, Guid, VariableName);
    } else if (VariableName == NULL && Guid == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND), gShellDebug1HiiHandle);
    } else if (VariableName == NULL && Guid != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMPSTORE_NO_VAR_FOUND_G), gShellDebug1HiiHandle, Guid);
    } 
    return (SHELL_NOT_FOUND);
  }
  return (SHELL_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-d", TypeFlag},
  {L"-l", TypeFlag},
  {L"-s", TypeFlag},
  {L"-all", TypeFlag},
  {L"-guid", TypeValue},
  {NULL, TypeMax}
  };

/**
  Function for 'dmpstore' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDmpStore (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *Temp;
  EFI_GUID      *Guid;
  EFI_GUID      GuidData;
  CONST CHAR16  *VariableName;

  ShellStatus   = SHELL_SUCCESS;
  Package       = NULL;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-all") && ShellCommandLineGetFlag(Package, L"-guid")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"-all", L"-guid");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag(Package, L"-s") || ShellCommandLineGetFlag(Package, L"-l")) && ShellCommandLineGetFlag(Package, L"-d")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDebug1HiiHandle, L"-l or -s", L"-d");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (!ShellCommandLineGetFlag(Package, L"-all")) {
        Temp = ShellCommandLineGetValue(Package, L"-guid");
        if (Temp != NULL) {
          Status = ConvertStringToGuid(Temp, &GuidData);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
          Guid = &GuidData;
        } else  {
          Guid = &gEfiGlobalVariableGuid;
        }
        VariableName = ShellCommandLineGetRawValue(Package, 1);
      } else {
        VariableName  = NULL;
        Guid          = NULL;
      }
      if (ShellStatus == SHELL_SUCCESS) {
        if (ShellCommandLineGetFlag(Package, L"-s") || ShellCommandLineGetFlag(Package, L"-l")) {
          ///@todo fix this after lib ready...
          ShellPrintEx(-1, -1, L"Not implemeneted yet.\r\n");
          ShellStatus = SHELL_UNSUPPORTED;
        } else {
          ShellStatus = ProcessVariables (VariableName, Guid, ShellCommandLineGetFlag(Package, L"-d"));
        }
      }
    }
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  return ShellStatus;
}

