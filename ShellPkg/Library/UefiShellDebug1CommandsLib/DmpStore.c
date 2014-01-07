/** @file
  Main file for DmpStore shell Debug1 function.

  Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"

/**
  Base on the input attribute value to return the attribute string.

  @param[in]     Atts           The input attribute value

  @retval The attribute string info.
**/
CHAR16 *
EFIAPI
GetAttrType (
  IN CONST UINT32 Atts
  )
{
  UINTN  BufLen;
  CHAR16 *RetString;

  BufLen      = 0;
  RetString   = NULL;
 
  if ((Atts & EFI_VARIABLE_NON_VOLATILE) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+NV", 0);
  }
  if ((Atts & EFI_VARIABLE_RUNTIME_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+RS+BS", 0);
  } else if ((Atts & EFI_VARIABLE_BOOTSERVICE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+BS", 0);
  }
  if ((Atts & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+HR", 0);
  }
  if ((Atts & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+AW", 0);
  }
  if ((Atts & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    StrnCatGrow (&RetString, &BufLen, L"+AT", 0);
  }

  if (RetString == NULL) {
    RetString = StrnCatGrow(&RetString, &BufLen, L"Invalid", 0);
  }

  if ((RetString != NULL) && (RetString[0] == L'+')) {
    CopyMem(RetString, RetString + 1, StrSize(RetString + 1));
  }

  return RetString;
}

/**
  Recursive function to display or delete variables.

  This function will call itself to create a stack-based list of allt he variables to process, 
  then fromt he last to the first, they will do either printing or deleting.

  This is necessary since once a delete happens GetNextVariableName() will work.

  @param[in] VariableName   The variable name of the EFI variable (or NULL).
  @param[in] Guid           The GUID of the variable set (or NULL).
  @param[in] Delete         TRUE to delete, FALSE otherwise.
  @param[in] PrevName       The previous variable name from GetNextVariableName. L"" to start.
  @param[in] FoundVarGuid   The previous GUID from GetNextVariableName. ignored at start.
  @param[in] FoundOne       If a VariableName or Guid was specified and one was printed or
                            deleted, then set this to TRUE, otherwise ignored.

  @retval SHELL_SUCCESS           The operation was successful.
  @retval SHELL_OUT_OF_RESOURCES  A memorty allocation failed.
  @retval SHELL_ABORTED           The abort message was received.
  @retval SHELL_DEVICE_ERROR      UEFI Variable Services returned an error.
  @retval SHELL_NOT_FOUND         the Name/Guid pair could not be found.
**/
SHELL_STATUS
EFIAPI
CascadeProcessVariables (
  IN CONST CHAR16   *VariableName OPTIONAL,
  IN CONST EFI_GUID *Guid OPTIONAL,
  IN BOOLEAN        Delete,
  IN CONST CHAR16   * CONST PrevName,
  IN EFI_GUID       FoundVarGuid,
  IN BOOLEAN        *FoundOne
  )
{
  EFI_STATUS                Status;
  CHAR16                    *FoundVarName;
  UINT8                     *DataBuffer;
  UINTN                     DataSize;
  UINT32                    Atts;
  SHELL_STATUS              ShellStatus;
  UINTN                     NameSize;
  CHAR16                    *RetString;

  if (ShellGetExecutionBreakFlag()) {
    return (SHELL_ABORTED);
  }

  NameSize      = 0;
  FoundVarName  = NULL;

  if (PrevName!=NULL) {
    StrnCatGrow(&FoundVarName, &NameSize, PrevName, 0);
  } else {
    FoundVarName = AllocateZeroPool(sizeof(CHAR16));
  }

  Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    SHELL_FREE_NON_NULL(FoundVarName);
    FoundVarName = AllocateZeroPool (NameSize);
    if (PrevName != NULL) {
      StrCpy(FoundVarName, PrevName);
    }

    Status = gRT->GetNextVariableName (&NameSize, FoundVarName, &FoundVarGuid);
  }

  //
  // No more is fine.
  //
  if (Status == EFI_NOT_FOUND) {
    SHELL_FREE_NON_NULL(FoundVarName);
    return (SHELL_SUCCESS);
  } else if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(FoundVarName);
    return (SHELL_DEVICE_ERROR);
  }

  //
  // Recurse to the next iteration.  We know "our" variable's name.
  //
  ShellStatus = CascadeProcessVariables(VariableName, Guid, Delete, FoundVarName, FoundVarGuid, FoundOne);

  //
  // No matter what happened we process our own variable
  // Only continue if Guid and VariableName are each either NULL or a match
  //
  if ( ( VariableName == NULL 
      || gUnicodeCollation->MetaiMatch(gUnicodeCollation, FoundVarName, (CHAR16*)VariableName) )
     && ( Guid == NULL 
      || CompareGuid(&FoundVarGuid, Guid) )
      ) {
    DataSize      = 0;
    DataBuffer    = NULL;
    //
    // do the print or delete
    //
    *FoundOne = TRUE;
    Status = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      SHELL_FREE_NON_NULL (DataBuffer);
      DataBuffer = AllocatePool (DataSize);
      if (DataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      } else {
        Status = gRT->GetVariable (FoundVarName, &FoundVarGuid, &Atts, &DataSize, DataBuffer);
      }
    }
    if (!Delete) {
      //
      // Last error check then print this variable out.
      //
      if (!EFI_ERROR(Status) && DataBuffer != NULL) {
        RetString = GetAttrType(Atts);
        ShellPrintHiiEx(
          -1,
          -1,
          NULL,
          STRING_TOKEN(STR_DMPSTORE_HEADER_LINE),
          gShellDebug1HiiHandle,
          RetString,
          &FoundVarGuid,
          FoundVarName,
          DataSize);
        DumpHex(2, 0, DataSize, DataBuffer);
        SHELL_FREE_NON_NULL(RetString);
      }
    } else {
      //
      // We only need name to delete it...
      //
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
    }
    SHELL_FREE_NON_NULL(DataBuffer);
  }

  SHELL_FREE_NON_NULL(FoundVarName);

  if (Status == EFI_DEVICE_ERROR) {
    ShellStatus = SHELL_DEVICE_ERROR;
  } else if (Status == EFI_SECURITY_VIOLATION) {
    ShellStatus = SHELL_SECURITY_VIOLATION;
  } else if (EFI_ERROR(Status)) {
    ShellStatus = SHELL_NOT_READY;
  }

  return (ShellStatus);
}

/**
  Function to display or delete variables.  This will set up and call into the recursive function.

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
  SHELL_STATUS              ShellStatus;
  BOOLEAN                   Found;
  EFI_GUID                  FoundVarGuid;

  Found         = FALSE;
  ShellStatus   = SHELL_SUCCESS;
  ZeroMem (&FoundVarGuid, sizeof(EFI_GUID));

  ShellStatus = CascadeProcessVariables(VariableName, Guid, Delete, NULL, FoundVarGuid, &Found);

  if (!Found) {
    if (ShellStatus == SHELL_OUT_OF_RESOURCES) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle);
      return (ShellStatus);
    } else if (VariableName != NULL && Guid == NULL) {
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
  return (ShellStatus);
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

