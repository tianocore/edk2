/** @file
  Main file for Dh shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-p", TypeValue},
  {L"-d", TypeFlag},
  {L"-v", TypeFlag},
  {L"-verbose", TypeFlag},
  {L"-sfo", TypeFlag},
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

STATIC CONST EFI_GUID *UefiDriverModelProtocolsGuidArray[] = {
  &gEfiDriverBindingProtocolGuid,
  &gEfiPlatformDriverOverrideProtocolGuid,
  &gEfiBusSpecificDriverOverrideProtocolGuid,
  &gEfiDriverDiagnosticsProtocolGuid,
  &gEfiDriverDiagnostics2ProtocolGuid,
  &gEfiComponentNameProtocolGuid,
  &gEfiComponentName2ProtocolGuid,
  &gEfiPlatformToDriverConfigurationProtocolGuid,
  &gEfiDriverSupportedEfiVersionProtocolGuid,
  &gEfiDriverFamilyOverrideProtocolGuid,
  &gEfiDriverHealthProtocolGuid,
  &gEfiLoadedImageProtocolGuid,
  NULL
};

BOOLEAN
EFIAPI
IsDriverProt (
  IN CONST EFI_GUID *Guid
  )
{
  CONST EFI_GUID            **GuidWalker;
  BOOLEAN                   GuidFound;
  GuidFound = FALSE;
  for (GuidWalker = UefiDriverModelProtocolsGuidArray
    ;  GuidWalker != NULL && *GuidWalker != NULL
    ;  GuidWalker++
   ){
    if (CompareGuid(*GuidWalker, Guid)) {
      GuidFound = TRUE;
      break;
    }
  }
  return (GuidFound);
}

CHAR16*
EFIAPI
GetProtocolInfoString(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST CHAR8      *Language,
  IN CONST CHAR16     *Seperator,
  IN CONST BOOLEAN    DriverInfo,
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    ExtraInfo
  )
{
  EFI_GUID                  **ProtocolGuidArray;
  UINTN                     ArrayCount;
  UINTN                     ProtocolIndex;
  EFI_STATUS                Status;
  CHAR16                    *RetVal;
  UINTN                     Size;
  CHAR16                    *Temp;

  ProtocolGuidArray = NULL;

  Status = gBS->ProtocolsPerHandle (
                TheHandle,
                &ProtocolGuidArray,
                &ArrayCount
               );
  if (!EFI_ERROR (Status)) {
    RetVal = NULL;
    Size   = 0;
    for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
      Temp = GetStringNameFromGuid(ProtocolGuidArray[ProtocolIndex], Language);
      if (Temp != NULL) {
        ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
        if (Size != 0) {
          StrnCatGrow(&RetVal, &Size, Seperator, 0);
        }
        StrnCatGrow(&RetVal, &Size, L"%H", 0);
        StrnCatGrow(&RetVal, &Size, Temp, 0);
        StrnCatGrow(&RetVal, &Size, L"%N", 0);
        FreePool(Temp);
      }
      if (ExtraInfo) {
        Temp = GetProtocolInformationDump(TheHandle, ProtocolGuidArray[ProtocolIndex], Verbose);
        if (Temp != NULL) {
          ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
          if (!Verbose) {
            StrnCatGrow(&RetVal, &Size, L"(", 0);
            StrnCatGrow(&RetVal, &Size, Temp, 0);
            StrnCatGrow(&RetVal, &Size, L")", 0);
          } else {
            StrnCatGrow(&RetVal, &Size, Seperator, 0);
            StrnCatGrow(&RetVal, &Size, Temp, 0);
          }
          FreePool(Temp);
        }
      }
    }
  } else {
    return (NULL);
  }

  if (ProtocolGuidArray != NULL) {
    FreePool(ProtocolGuidArray);
  }
  ASSERT((RetVal == NULL && Size == 0) || (RetVal != NULL));
  StrnCatGrow(&RetVal, &Size, Seperator, 0);
  return (RetVal);
}

SHELL_STATUS
EFIAPI
DoDhByHandle(
  IN CONST EFI_HANDLE TheHandle,
  IN CONST BOOLEAN    Verbose,
  IN CONST BOOLEAN    Sfo,
  IN CONST CHAR8      *Language,
  IN CONST BOOLEAN    DriverInfo,
  IN CONST BOOLEAN    Multiple
  )
{
  CHAR16              *ProtocolInfoString;
  SHELL_STATUS        ShellStatus;
  EFI_STATUS          Status;

  Status              = EFI_SUCCESS;
  ShellStatus         = SHELL_SUCCESS;
  ProtocolInfoString  = NULL;

  if (!Sfo) {
    if (Multiple) {
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L" ", DriverInfo, Verbose, TRUE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex(TheHandle),
        ProtocolInfoString==NULL?L"":ProtocolInfoString);
    } else {
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L"\r\n", DriverInfo, Verbose, TRUE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_SINGLE),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex(TheHandle),
        TheHandle,
        ProtocolInfoString==NULL?L"":ProtocolInfoString);
    }
  } else {
//#string STR_DH_OUTPUT_SFO         #language en-US "%s, %s, %s, %H%02x%N, %s, %s\r\n"
      ProtocolInfoString = GetProtocolInfoString(TheHandle, Language, L";", DriverInfo, FALSE, FALSE);
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DH_OUTPUT_SFO),
        gShellDriver1HiiHandle,
        Multiple ?L"HandlesInfo":L"HandleInfo",
        L"DriverName",
        L"ControllerName",
        ConvertHandleToHandleIndex(TheHandle),
        L"DevPath",
        ProtocolInfoString==NULL?L"":ProtocolInfoString);


  }


  if (ProtocolInfoString != NULL) {
    FreePool(ProtocolInfoString);
  }
  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
DoDhForHandleList(
  IN CONST EFI_HANDLE *HandleList,
  IN CONST BOOLEAN    Sfo,
  IN CONST CHAR8      *Language,
  IN CONST BOOLEAN    DriverInfo
  )
{
  CONST EFI_HANDLE  *HandleWalker;
  SHELL_STATUS      ShellStatus;

  ShellStatus       = SHELL_SUCCESS;

  for (HandleWalker = HandleList ; HandleWalker != NULL && *HandleWalker != NULL && ShellStatus == SHELL_SUCCESS; HandleWalker++) {
    ShellStatus = DoDhByHandle(
          *HandleWalker,
          FALSE,
          Sfo,
          Language,
          DriverInfo,
          TRUE
         );
  }
  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
DoDhForAll(
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  EFI_HANDLE    *HandleList;
  SHELL_STATUS  ShellStatus;

  HandleList = GetHandleListByProtocol(NULL);

  ShellStatus = DoDhForHandleList(
    HandleList,
    Sfo,
    Language,
    DriverInfo);

  FreePool(HandleList);

  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
DoDhByProtocol(
  IN CONST CHAR16   *ProtocolName,
  IN CONST BOOLEAN  Sfo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  DriverInfo
  )
{
  EFI_GUID      *Guid;
  EFI_STATUS    Status;
  EFI_HANDLE    *HandleList;
  SHELL_STATUS  ShellStatus;

  ASSERT(ProtocolName != NULL);

  Status = GetGuidFromStringName(ProtocolName, Language, &Guid);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DH_NO_GUID_FOUND), gShellDriver1HiiHandle, ProtocolName);
    return (SHELL_INVALID_PARAMETER);
  }

  HandleList = GetHandleListByProtocol(Guid);

  ShellStatus = DoDhForHandleList(
    HandleList,
    Sfo,
    Language,
    DriverInfo);

  SHELL_FREE_NON_NULL(HandleList);

  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
ShellCommandRunDh (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR8               *Language;
  CONST CHAR16        *Lang;
  CONST CHAR16        *Temp2;
  BOOLEAN             SfoMode;
  BOOLEAN             FlagD;
  BOOLEAN             Verbose;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
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
    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellCommandLineFreeVarList (Package);
      return (SHELL_INVALID_PARAMETER);
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

    SfoMode = ShellCommandLineGetFlag(Package, L"-sfo");
    FlagD   = ShellCommandLineGetFlag(Package, L"-d");
    if (ShellCommandLineGetFlag(Package, L"-v") || ShellCommandLineGetFlag(Package, L"-verbose")) {
      Verbose = TRUE;
    } else {
      Verbose = FALSE;
    }

    if (ShellCommandLineGetFlag(Package, L"-p")) {
      if (ShellCommandLineGetCount(Package) > 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetValue(Package, L"-p") == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-p");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // print by protocol
        //
        ShellStatus = DoDhByProtocol(
          ShellCommandLineGetValue(Package, L"-p"),
          SfoMode,
          Lang==NULL?NULL:Language,
          FlagD
         );
      }
    } else {
      Temp2 = ShellCommandLineGetRawValue(Package, 1);
      if (Temp2 == NULL) {
        //
        // Print everything
        //
        ShellStatus = DoDhForAll(
          SfoMode,
          Lang==NULL?NULL:Language,
          FlagD
         );
      } else {
        if (!ShellIsHexOrDecimalNumber(Temp2, TRUE, FALSE) || ConvertHandleIndexToHandle(StrHexToUintn(Temp2)) == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Temp2);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // print 1 handle
          //
          ShellStatus = DoDhByHandle(
            ConvertHandleIndexToHandle(StrHexToUintn(Temp2)),
            Verbose,
            SfoMode,
            Lang==NULL?NULL:Language,
            FlagD,
            FALSE
           );
        }
      }
    }


    ShellCommandLineFreeVarList (Package);
    SHELL_FREE_NON_NULL(Language);
  }

  return (ShellStatus);
}
