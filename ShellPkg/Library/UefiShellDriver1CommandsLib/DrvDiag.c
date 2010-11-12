/** @file
  Main file for DrvDiag shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST EFI_GUID *DiagGuidList[] = {&gEfiDriverDiagnosticsProtocolGuid, &gEfiDriverDiagnostics2ProtocolGuid, NULL};
//
// We need 1 more item on the list...
//
typedef enum {
  TEST_MODE_STANDARD      = EfiDriverDiagnosticTypeStandard,
  TEST_MODE_EXTENDED      = EfiDriverDiagnosticTypeExtended,
  TEST_MODE_MANUFACTURING = EfiDriverDiagnosticTypeManufacturing,
  TEST_MODE_LIST,
  TEST_MODE_MAX
} DRV_DIAG_TEST_MODE;

EFI_STATUS
EFIAPI
DoDiagnostics (
  IN CONST DRV_DIAG_TEST_MODE Mode,
  IN CONST CHAR8              *Lang,
  IN CONST BOOLEAN            AllChilds,
  IN CONST EFI_HANDLE         DriverHandle,
  IN CONST EFI_HANDLE         ControllerHandle,
  IN CONST EFI_HANDLE         ChildHandle
  )
{
  EFI_DRIVER_DIAGNOSTICS_PROTOCOL     *DriverDiagnostics;
  EFI_DRIVER_DIAGNOSTICS2_PROTOCOL    *DriverDiagnostics2;
  EFI_HANDLE                          *DriverHandleList;
  EFI_HANDLE                          *ControllerHandleList;
  EFI_HANDLE                          *ChildHandleList;
  EFI_HANDLE                          *Walker;
  UINTN                               DriverHandleListCount;
  UINTN                               ControllerHandleListCount;
  UINTN                               ChildHandleListCount;
  UINTN                               DriverHandleListLoop;
  UINTN                               ControllerHandleListLoop;
  UINTN                               ChildHandleListLoop;
  EFI_STATUS                          Status;
  EFI_STATUS                          Status2;
  EFI_GUID                            *ErrorType;
  UINTN                               OutBufferSize;
  CHAR16                              *OutBuffer;
  UINTN                               HandleIndex1;
  UINTN                               HandleIndex2;

  if ((ChildHandle != NULL && AllChilds) || (Mode >= TEST_MODE_MAX)){
    return (EFI_INVALID_PARAMETER);
  }

  if (Lang == NULL || AsciiStrLen(Lang) < 3) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"-l <value>");
    return (EFI_INVALID_PARAMETER);
  }

  DriverDiagnostics                   = NULL;
  DriverDiagnostics2                  = NULL;
  Status                              = EFI_SUCCESS;
  Status2                             = EFI_SUCCESS;
  DriverHandleList                    = NULL;
  ControllerHandleList                = NULL;
  ChildHandleList                     = NULL;
  OutBuffer                           = NULL;
  ErrorType                           = NULL;
  DriverHandleListCount               = 0;
  ControllerHandleListCount           = 0;
  ChildHandleListCount                = 0;

  if (DriverHandle != NULL) {
    DriverHandleList = AllocateZeroPool(2*sizeof(EFI_HANDLE));
    ASSERT(DriverHandleList!=NULL);
    DriverHandleList[0] = DriverHandle;
    DriverHandleListCount = 1;
  } else {
    DriverHandleList = GetHandleListByProtocolList(DiagGuidList);
    if (DriverHandleList == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_NONE), gShellDriver1HiiHandle);
      return (EFI_NOT_FOUND);
    } 
    for (Walker = DriverHandleList ; Walker != NULL && *Walker != NULL ; DriverHandleListCount++, Walker++);
  }

  if (ControllerHandle != NULL) {
    ControllerHandleList = AllocateZeroPool(2*sizeof(EFI_HANDLE));
    ASSERT(ControllerHandleList!=NULL);
    ControllerHandleList[0] = ControllerHandle;
    ControllerHandleListCount = 1;
  } else {
    ControllerHandleList = NULL;
  }

  if (ChildHandle != NULL) {
    ChildHandleList = AllocateZeroPool(2*sizeof(EFI_HANDLE));
    ASSERT(ChildHandleList!=NULL);
    ChildHandleList[0] = ChildHandle;
    ChildHandleListCount = 1;
  } else if (AllChilds) {
    ChildHandleList = NULL;
    //
    // This gets handled in the loop below.
    //
  } else {
    ChildHandleList = NULL;
  }

  for (DriverHandleListLoop = 0
    ;  DriverHandleListLoop < DriverHandleListCount
    ;  DriverHandleListLoop++
    ){
    if (ControllerHandle == NULL) {
      PARSE_HANDLE_DATABASE_DEVICES(DriverHandleList[DriverHandleListLoop], &ControllerHandleListCount, &ControllerHandleList);
    }
    for (ControllerHandleListLoop = 0
      ;  ControllerHandleListLoop < ControllerHandleListCount
      ;  ControllerHandleListLoop++
     ){
      if (AllChilds) {
        ASSERT(ChildHandleList == NULL);
        PARSE_HANDLE_DATABASE_MANAGED_CHILDREN(
          DriverHandleList[DriverHandleListLoop], 
          ControllerHandleList[ControllerHandleListLoop],
          &ChildHandleListCount,
          &ChildHandleList);
      }
      for (ChildHandleListLoop = 0
        ;  (ChildHandleListLoop < ChildHandleListCount || ChildHandleList == NULL)
        ;  ChildHandleListLoop++
        ){
        if (Mode != TEST_MODE_LIST) {
          if (Lang[2] == '-') {
            //
            // Get the protocol pointer and call the function
            //
            Status = gBS->OpenProtocol(
              DriverHandleList[DriverHandleListLoop],
              &gEfiDriverDiagnostics2ProtocolGuid,
              (VOID**)&DriverDiagnostics2,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            if (!EFI_ERROR(Status)) {
              Status = DriverDiagnostics2->RunDiagnostics(
                DriverDiagnostics2,
                ControllerHandleList[ControllerHandleListLoop],
                ChildHandleList == NULL?NULL:ChildHandleList[ChildHandleListLoop],
                (EFI_DRIVER_DIAGNOSTIC_TYPE)Mode,
                (CHAR8*)Lang,
                &ErrorType,
                &OutBufferSize,
                &OutBuffer);
            }
          } else {
            Status = gBS->OpenProtocol(
              DriverHandleList[DriverHandleListLoop],
              &gEfiDriverDiagnosticsProtocolGuid,
              (VOID**)&DriverDiagnostics,
              gImageHandle,
              NULL,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            if (!EFI_ERROR(Status)) {
              Status = DriverDiagnostics->RunDiagnostics(
                DriverDiagnostics,
                ControllerHandleList[ControllerHandleListLoop],
                ChildHandleList == NULL?NULL:ChildHandleList[ChildHandleListLoop],
                (EFI_DRIVER_DIAGNOSTIC_TYPE)Mode,
                (CHAR8*)Lang,
                &ErrorType,
                &OutBufferSize,
                &OutBuffer);
            }
          }
          if (EFI_ERROR(Status)) {
            Status2 = Status;
          }
          HandleIndex1 = ConvertHandleToHandleIndex(DriverHandleList[DriverHandleListLoop]);
          HandleIndex2 = ConvertHandleToHandleIndex(ControllerHandleList[ControllerHandleListLoop]);
          ShellPrintHiiEx(
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_3P_RESULT),
            gShellDriver1HiiHandle,
            L"DrvDiag",
            HandleIndex1,
            HandleIndex2,
            ChildHandleList == NULL?0:ConvertHandleToHandleIndex(ChildHandleList[ChildHandleListLoop]),
            Status);
          if (OutBuffer!=NULL) {
            FreePool(OutBuffer);
            OutBuffer = NULL;
          }
          if (ErrorType!=NULL) {
            FreePool(ErrorType);
            ErrorType = NULL;
          }
        } else {
          HandleIndex1 = ConvertHandleToHandleIndex(DriverHandleList[DriverHandleListLoop]);
          HandleIndex2 = ConvertHandleToHandleIndex(ControllerHandleList[ControllerHandleListLoop]);
          //
          // Print out the information that this set can be tested
          //
          ShellPrintHiiEx(
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DRV_DIAG_ITEM_LINE),
            gShellDriver1HiiHandle,
            HandleIndex1,
            HandleIndex2,
            ChildHandleList == NULL?0:ConvertHandleToHandleIndex(ChildHandleList[ChildHandleListLoop])
         );
        }

        //
        // If we are doing a single pass with NULL child jump out after a single loop
        //
        if (ChildHandleList == NULL) {
          break;
        }
      }
      if (AllChilds) {
        SHELL_FREE_NON_NULL(ChildHandleList);
        ChildHandleList       = NULL;
        ChildHandleListCount  = 0;
      }
    }
    if (ControllerHandle == NULL) {
      SHELL_FREE_NON_NULL(ControllerHandleList);
      ControllerHandleList      = NULL;
      ControllerHandleListCount = 0;
    }
  }

  if (DriverHandleList != NULL) {
    FreePool(DriverHandleList);
  }
  if (ControllerHandleList != NULL) {
    FreePool(ControllerHandleList);
  }
  if (ChildHandleList != NULL) {
    FreePool(ChildHandleList);
  }
  return (Status2);
}


STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c", TypeFlag},
  {L"-s", TypeFlag},
  {L"-e", TypeFlag},
  {L"-m", TypeFlag},
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

SHELL_STATUS
EFIAPI
ShellCommandRunDrvDiag (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  DRV_DIAG_TEST_MODE  Mode;
  CHAR8               *Language;
  CONST CHAR16        *DriverHandleStr;
  CONST CHAR16        *ControllerHandleStr;
  CONST CHAR16        *ChildHandleStr;
  CONST CHAR16        *Lang;
  EFI_HANDLE          Handle1;
  EFI_HANDLE          Handle2;
  EFI_HANDLE          Handle3;

  ShellStatus         = SHELL_SUCCESS;
  Mode                = TEST_MODE_MAX;
  Language            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // if more than 3 'value' parameters (plus the name one) or we have any 2 mode flags
    //
    if ((ShellCommandLineGetCount(Package) > 4)
      ||(ShellCommandLineGetFlag(Package, L"-s") && ShellCommandLineGetFlag(Package, L"-e"))
      ||(ShellCommandLineGetFlag(Package, L"-s") && ShellCommandLineGetFlag(Package, L"-m"))
      ||(ShellCommandLineGetFlag(Package, L"-e") && ShellCommandLineGetFlag(Package, L"-m"))
     ){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag(Package, L"-s"))
            || (ShellCommandLineGetFlag(Package, L"-e"))
            || (ShellCommandLineGetFlag(Package, L"-m"))
           ){
      //
      // Run the apropriate test
      //
      if        (ShellCommandLineGetFlag(Package, L"-s")) {
        Mode =   TEST_MODE_STANDARD;
      } else if (ShellCommandLineGetFlag(Package, L"-e")) {
        Mode = TEST_MODE_EXTENDED;
      } else if (ShellCommandLineGetFlag(Package, L"-m")) {
        Mode = TEST_MODE_MANUFACTURING;
      } else {
        ASSERT(FALSE);
      }
    } else {
      //
      // Do a listing of what's available to test
      //
      Mode = TEST_MODE_LIST;
    }

    Lang = ShellCommandLineGetValue(Package, L"-l");
    if (Lang != NULL) {
      Language = AllocateZeroPool(StrSize(Lang));
      AsciiSPrint(Language, StrSize(Lang), "%S", Lang);
    } else if (!ShellCommandLineGetFlag(Package, L"-l")){
      Language = AllocateZeroPool(10);
      AsciiSPrint(Language, 10, "en-us");
    } else {
      ASSERT(Language == NULL);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-l");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    }

    DriverHandleStr     = ShellCommandLineGetRawValue(Package, 1);
    ControllerHandleStr = ShellCommandLineGetRawValue(Package, 2);
    ChildHandleStr      = ShellCommandLineGetRawValue(Package, 3);

    if (DriverHandleStr == NULL) {
      Handle1 = NULL;
    } else {
      Handle1 = ConvertHandleIndexToHandle(StrHexToUintn(DriverHandleStr    ));
    }
    if (ControllerHandleStr == NULL) {
      Handle2 = NULL;
    } else {
      Handle2 = ConvertHandleIndexToHandle(StrHexToUintn(ControllerHandleStr));
    }
    if (ChildHandleStr == NULL) {
      Handle3 = NULL;
    } else {
      Handle3 = ConvertHandleIndexToHandle(StrHexToUintn(ChildHandleStr     ));
    }

    Status = DoDiagnostics (
      Mode,
      Language,
      ShellCommandLineGetFlag(Package, L"-c"),
      Handle1, 
      Handle2, 
      Handle3
      );

    SHELL_FREE_NON_NULL(Language);
    ShellCommandLineFreeVarList (Package);

  }
  if (ShellStatus == SHELL_SUCCESS) {
    if (Status == EFI_SECURITY_VIOLATION) {
      ShellStatus = SHELL_SECURITY_VIOLATION;
    } else if (Status == EFI_INVALID_PARAMETER) {
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (Status == EFI_NOT_FOUND) {
      ShellStatus = SHELL_NOT_FOUND;
    } else if (EFI_ERROR(Status)) {
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  return (ShellStatus);
}
