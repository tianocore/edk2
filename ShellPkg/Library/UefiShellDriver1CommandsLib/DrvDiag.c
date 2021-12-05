/** @file
  Main file for DrvDiag shell Driver1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST EFI_GUID  *DiagGuidList[] = { &gEfiDriverDiagnosticsProtocolGuid, &gEfiDriverDiagnostics2ProtocolGuid, NULL };
//
// We need 1 more item on the list...
//
typedef enum {
  TestModeStandard      = EfiDriverDiagnosticTypeStandard,
  TestModeExtended      = EfiDriverDiagnosticTypeExtended,
  TestModeManufacturing = EfiDriverDiagnosticTypeManufacturing,
  TestModeList,
  TestModeMax
} DRV_DIAG_TEST_MODE;

/**
  Do the diagnostics call for some set of handles.

  @param[in] Mode               The type of diagnostic test to run.
  @param[in] Lang               The language code to use.
  @param[in] AllChilds          Should the test be on all children.
  @param[in] DriverHandle       The driver handle to test with.
  @param[in] ControllerHandle   The specific controller handle to test.
  @param[in] ChildHandle        The specific child handle to test.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_INVALID_PARAMETER A parameter had an invalid value.
  @retval EFI_NOT_FOUND         No diagnostic handle could be found.
**/
EFI_STATUS
DoDiagnostics (
  IN CONST DRV_DIAG_TEST_MODE  Mode,
  IN CONST CHAR8               *Lang,
  IN CONST BOOLEAN             AllChilds,
  IN CONST EFI_HANDLE          DriverHandle,
  IN CONST EFI_HANDLE          ControllerHandle,
  IN CONST EFI_HANDLE          ChildHandle
  )
{
  EFI_DRIVER_DIAGNOSTICS_PROTOCOL   *DriverDiagnostics;
  EFI_DRIVER_DIAGNOSTICS2_PROTOCOL  *DriverDiagnostics2;
  EFI_HANDLE                        *DriverHandleList;
  EFI_HANDLE                        *ControllerHandleList;
  EFI_HANDLE                        *ChildHandleList;
  EFI_HANDLE                        *Walker;
  UINTN                             DriverHandleListCount;
  UINTN                             ControllerHandleListCount;
  UINTN                             ChildHandleListCount;
  UINTN                             DriverHandleListLoop;
  UINTN                             ControllerHandleListLoop;
  UINTN                             ChildHandleListLoop;
  EFI_STATUS                        Status;
  EFI_STATUS                        Status2;
  EFI_GUID                          *ErrorType;
  UINTN                             OutBufferSize;
  CHAR16                            *OutBuffer;
  UINTN                             HandleIndex1;
  UINTN                             HandleIndex2;
  CHAR8                             *Language;
  BOOLEAN                           Found;

  if (((ChildHandle != NULL) && AllChilds) || (Mode >= TestModeMax)) {
    return (EFI_INVALID_PARAMETER);
  }

  DriverDiagnostics         = NULL;
  DriverDiagnostics2        = NULL;
  Status                    = EFI_SUCCESS;
  Status2                   = EFI_SUCCESS;
  DriverHandleList          = NULL;
  ControllerHandleList      = NULL;
  ChildHandleList           = NULL;
  Language                  = NULL;
  OutBuffer                 = NULL;
  ErrorType                 = NULL;
  DriverHandleListCount     = 0;
  ControllerHandleListCount = 0;
  ChildHandleListCount      = 0;

  if (DriverHandle != NULL) {
    DriverHandleList = AllocateZeroPool (2*sizeof (EFI_HANDLE));
    if (DriverHandleList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DriverHandleList[0]   = DriverHandle;
    DriverHandleListCount = 1;
  } else {
    DriverHandleList = GetHandleListByProtocolList (DiagGuidList);
    if (DriverHandleList == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROTOCOL_NF), gShellDriver1HiiHandle, L"drvdiag", L"gEfiDriverDiagnosticsProtocolGuid", &gEfiDriverDiagnosticsProtocolGuid);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROTOCOL_NF), gShellDriver1HiiHandle, L"drvdiag", L"gEfiDriverDiagnostics2ProtocolGuid", &gEfiDriverDiagnostics2ProtocolGuid);
      return (EFI_NOT_FOUND);
    }

    for (Walker = DriverHandleList; Walker != NULL && *Walker != NULL; DriverHandleListCount++, Walker++) {
    }
  }

  if (ControllerHandle != NULL) {
    ControllerHandleList = AllocateZeroPool (2*sizeof (EFI_HANDLE));
    if (ControllerHandleList == NULL) {
      SHELL_FREE_NON_NULL (DriverHandleList);
      return EFI_OUT_OF_RESOURCES;
    }

    ControllerHandleList[0]   = ControllerHandle;
    ControllerHandleListCount = 1;
  } else {
    ControllerHandleList = NULL;
  }

  if (ChildHandle != NULL) {
    ChildHandleList = AllocateZeroPool (2*sizeof (EFI_HANDLE));
    if (ChildHandleList == NULL) {
      SHELL_FREE_NON_NULL (ControllerHandleList);
      SHELL_FREE_NON_NULL (DriverHandleList);
      return EFI_OUT_OF_RESOURCES;
    }

    ChildHandleList[0]   = ChildHandle;
    ChildHandleListCount = 1;
  } else if (AllChilds) {
    ChildHandleList = NULL;
    //
    // This gets handled in the loop below.
    //
  } else {
    ChildHandleList = NULL;
  }

  if (Mode == TestModeList) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVDIAG_HEADER), gShellDriver1HiiHandle);
  }

  for (DriverHandleListLoop = 0
       ; DriverHandleListLoop < DriverHandleListCount
       ; DriverHandleListLoop++
       )
  {
    if (Mode == TestModeList) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVDIAG_DRIVER_HEADER), gShellDriver1HiiHandle, ConvertHandleToHandleIndex (DriverHandleList[DriverHandleListLoop]));
    }

    if (ControllerHandle == NULL) {
      PARSE_HANDLE_DATABASE_DEVICES (DriverHandleList[DriverHandleListLoop], &ControllerHandleListCount, &ControllerHandleList);
    }

    if (ControllerHandleListCount == 0) {
      if (Mode == TestModeList) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVDIAG_DRIVER_NO_HANDLES), gShellDriver1HiiHandle);
      }
    } else {
      if (Mode == TestModeList) {
        ShellPrintEx (-1, -1, L"\r\n");
      }

      for (ControllerHandleListLoop = 0
           ; ControllerHandleListLoop < ControllerHandleListCount
           ; ControllerHandleListLoop++
           )
      {
        if (AllChilds) {
          ASSERT (ChildHandleList == NULL);
          PARSE_HANDLE_DATABASE_MANAGED_CHILDREN (
            DriverHandleList[DriverHandleListLoop],
            ControllerHandleList[ControllerHandleListLoop],
            &ChildHandleListCount,
            &ChildHandleList
            );
        }

        for (ChildHandleListLoop = 0
             ; (ChildHandleListLoop < ChildHandleListCount || ChildHandleList == NULL)
             ; ChildHandleListLoop++
             )
        {
          Found = FALSE;
          if (Mode != TestModeList) {
            if ((Lang == NULL) || (Lang[2] == '-')) {
              //
              // Get the protocol pointer and call the function
              //
              Status = gBS->OpenProtocol (
                              DriverHandleList[DriverHandleListLoop],
                              &gEfiDriverDiagnostics2ProtocolGuid,
                              (VOID **)&DriverDiagnostics2,
                              gImageHandle,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL
                              );
              if (!EFI_ERROR (Status) && (DriverDiagnostics2 != NULL)) {
                Language = GetBestLanguageForDriver (DriverDiagnostics2->SupportedLanguages, Lang, FALSE);
                Found    = TRUE;
                Status   = DriverDiagnostics2->RunDiagnostics (
                                                 DriverDiagnostics2,
                                                 ControllerHandleList[ControllerHandleListLoop],
                                                 ChildHandleList == NULL ? NULL : ChildHandleList[ChildHandleListLoop],
                                                 (EFI_DRIVER_DIAGNOSTIC_TYPE)Mode,
                                                 Language,
                                                 &ErrorType,
                                                 &OutBufferSize,
                                                 &OutBuffer
                                                 );
                FreePool (Language);
              }
            }

            if (!Found && ((Lang == NULL) || ((Lang != NULL) && (Lang[2] != '-')))) {
              Status = gBS->OpenProtocol (
                              DriverHandleList[DriverHandleListLoop],
                              &gEfiDriverDiagnosticsProtocolGuid,
                              (VOID **)&DriverDiagnostics,
                              gImageHandle,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL
                              );
              if (!EFI_ERROR (Status)) {
                Language = GetBestLanguageForDriver (DriverDiagnostics->SupportedLanguages, Lang, FALSE);
                Status   = DriverDiagnostics->RunDiagnostics (
                                                DriverDiagnostics,
                                                ControllerHandleList[ControllerHandleListLoop],
                                                ChildHandleList == NULL ? NULL : ChildHandleList[ChildHandleListLoop],
                                                (EFI_DRIVER_DIAGNOSTIC_TYPE)Mode,
                                                Language,
                                                &ErrorType,
                                                &OutBufferSize,
                                                &OutBuffer
                                                );
                FreePool (Language);
              }
            }

            if (EFI_ERROR (Status)) {
              Status2 = Status;
            }

            HandleIndex1 = ConvertHandleToHandleIndex (DriverHandleList[DriverHandleListLoop]);
            HandleIndex2 = ConvertHandleToHandleIndex (ControllerHandleList[ControllerHandleListLoop]);
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_3P_RESULT),
              gShellDriver1HiiHandle,
              L"DrvDiag",
              HandleIndex1,
              HandleIndex2,
              ChildHandleList == NULL ? 0 : ConvertHandleToHandleIndex (ChildHandleList[ChildHandleListLoop]),
              Status
              );
            if (OutBuffer != NULL) {
              FreePool (OutBuffer);
              OutBuffer = NULL;
            }

            if (ErrorType != NULL) {
              FreePool (ErrorType);
              ErrorType = NULL;
            }
          } else {
            HandleIndex1 = ConvertHandleToHandleIndex (DriverHandleList[DriverHandleListLoop]);
            HandleIndex2 = ConvertHandleToHandleIndex (ControllerHandleList[ControllerHandleListLoop]);
            //
            // Print out the information that this set can be tested
            //
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRV_DIAG_ITEM_LINE),
              gShellDriver1HiiHandle,
              HandleIndex1,
              HandleIndex2,
              ChildHandleList == NULL ? 0 : ConvertHandleToHandleIndex (ChildHandleList[ChildHandleListLoop])
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
          SHELL_FREE_NON_NULL (ChildHandleList);
          ChildHandleList      = NULL;
          ChildHandleListCount = 0;
        }
      }

      if (ControllerHandle == NULL) {
        SHELL_FREE_NON_NULL (ControllerHandleList);
        ControllerHandleList      = NULL;
        ControllerHandleListCount = 0;
      }
    }
  }

  if (DriverHandleList != NULL) {
    FreePool (DriverHandleList);
  }

  if (ControllerHandleList != NULL) {
    FreePool (ControllerHandleList);
  }

  if (ChildHandleList != NULL) {
    FreePool (ChildHandleList);
  }

  return (Status2);
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-c", TypeFlag  },
  { L"-s", TypeFlag  },
  { L"-e", TypeFlag  },
  { L"-m", TypeFlag  },
  { L"-l", TypeValue },
  { NULL,  TypeMax   }
};

/**
  Function for 'drvdiag' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
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
  UINT64              Intermediate;

  ShellStatus = SHELL_SUCCESS;
  Mode        = TestModeMax;
  Language    = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"drvdiag", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // if more than 3 'value' parameters (plus the name one) or we have any 2 mode flags
    //
    if (  (ShellCommandLineGetCount (Package) > 4)
       || (ShellCommandLineGetFlag (Package, L"-s") && ShellCommandLineGetFlag (Package, L"-e"))
       || (ShellCommandLineGetFlag (Package, L"-s") && ShellCommandLineGetFlag (Package, L"-m"))
       || (ShellCommandLineGetFlag (Package, L"-e") && ShellCommandLineGetFlag (Package, L"-m"))
          )
    {
      //
      // error for too many parameters
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"drvdiag");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (  (ShellCommandLineGetFlag (Package, L"-s"))
              || (ShellCommandLineGetFlag (Package, L"-e"))
              || (ShellCommandLineGetFlag (Package, L"-m"))
                 )
    {
      //
      // Run the appropriate test
      //
      if (ShellCommandLineGetFlag (Package, L"-s")) {
        Mode =   TestModeStandard;
      } else if (ShellCommandLineGetFlag (Package, L"-e")) {
        Mode = TestModeExtended;
      } else if (ShellCommandLineGetFlag (Package, L"-m")) {
        Mode = TestModeManufacturing;
      } else {
        ASSERT (FALSE);
      }
    } else {
      //
      // Do a listing of what's available to test
      //
      Mode = TestModeList;
    }

    Lang = ShellCommandLineGetValue (Package, L"-l");
    if (ShellCommandLineGetFlag (Package, L"-l") && (Lang == NULL)) {
      ASSERT (Language == NULL);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"drvdiag", L"-l");
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
    } else if (Lang != NULL) {
      Language = AllocateZeroPool (StrSize (Lang));
      AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
    }

    DriverHandleStr     = ShellCommandLineGetRawValue (Package, 1);
    ControllerHandleStr = ShellCommandLineGetRawValue (Package, 2);
    ChildHandleStr      = ShellCommandLineGetRawValue (Package, 3);

    if (DriverHandleStr == NULL) {
      Handle1 = NULL;
    } else {
      ShellConvertStringToUint64 (DriverHandleStr, &Intermediate, TRUE, FALSE);
      Handle1 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
    }

    if (ControllerHandleStr == NULL) {
      Handle2 = NULL;
    } else {
      ShellConvertStringToUint64 (ControllerHandleStr, &Intermediate, TRUE, FALSE);
      Handle2 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
    }

    if (ChildHandleStr == NULL) {
      Handle3 = NULL;
    } else {
      ShellConvertStringToUint64 (ChildHandleStr, &Intermediate, TRUE, FALSE);
      Handle3 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
    }

    Status = DoDiagnostics (
               Mode,
               Language,
               ShellCommandLineGetFlag (Package, L"-c"),
               Handle1,
               Handle2,
               Handle3
               );

    SHELL_FREE_NON_NULL (Language);
    ShellCommandLineFreeVarList (Package);
  }

  if (ShellStatus == SHELL_SUCCESS) {
    if (Status == EFI_SECURITY_VIOLATION) {
      ShellStatus = SHELL_SECURITY_VIOLATION;
    } else if (Status == EFI_INVALID_PARAMETER) {
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (Status == EFI_NOT_FOUND) {
      ShellStatus = SHELL_NOT_FOUND;
    } else if (EFI_ERROR (Status)) {
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  return (ShellStatus);
}
