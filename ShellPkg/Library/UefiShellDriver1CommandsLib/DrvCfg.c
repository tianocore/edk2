/** @file
  Main file for DrvCfg shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>

/**
  Function to validate configuration information on a configurable handle.

  @param[in] Handle           The handle to validate info on.
  @param[in] HiiDb            A pointer to the HII Database protocol.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
EFIAPI
ValidateConfigInfoOnSingleHandleHii(
  IN CONST EFI_HANDLE             Handle,
  IN EFI_HII_DATABASE_PROTOCOL    *HiiDb
  )
{
  EFI_STATUS                  Status;
  UINTN                       Size;
  EFI_HII_HANDLE              *HiiHandle;
  EFI_HII_HANDLE              *CurrentHandle;
  EFI_HANDLE                  NormalHandle;

  if (HiiDb == NULL || Handle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Size      = 0;
  HiiHandle = NULL;

  Status = HiiDb->ListPackageLists(
    HiiDb,
    EFI_HII_PACKAGE_TYPE_ALL,
    NULL,
    &Size,
    HiiHandle);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiHandle = AllocateZeroPool(Size);
    if (HiiHandle == NULL) {
      return (EFI_OUT_OF_RESOURCES);
    }
    Status = HiiDb->ListPackageLists(
      HiiDb,
      EFI_HII_PACKAGE_TYPE_ALL,
      NULL,
      &Size,
      HiiHandle);
  }
  if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(HiiHandle);
    return (Status);
  }

  for (CurrentHandle = HiiHandle ; CurrentHandle != NULL && *CurrentHandle != NULL ; CurrentHandle++) {
    NormalHandle = NULL;
    Status = HiiDb->GetPackageListHandle(
      HiiDb,
      *CurrentHandle,
      &NormalHandle);
    if (NormalHandle == Handle) {
      break;
    }
  }





  SHELL_FREE_NON_NULL(HiiHandle);
  return (Status);
}

/**
  Function to validate configuration information on all configurable handles.

  @param[in] ChildrenToo    TRUE to tewst for children.

  @retval SHELL_SUCCESS     The operation was successful.
**/
SHELL_STATUS
EFIAPI
ValidOptionsOnAll(
  IN CONST BOOLEAN ChildrenToo
  )
{
  EFI_HANDLE                  *HandleList;
  EFI_HANDLE                  *CurrentHandle;
  SHELL_STATUS                ShellStatus;
  EFI_STATUS                  Status;
  BOOLEAN                     Found;
  EFI_HII_DATABASE_PROTOCOL   *HiiDb;

  Found             = FALSE;
  HandleList        = NULL;
  ShellStatus       = SHELL_SUCCESS;
  Status            = EFI_SUCCESS;

  Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID**)&HiiDb);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROTOCOL_NF), gShellDriver1HiiHandle, L"gEfiHiiDatabaseProtocolGuid", &gEfiHiiDatabaseProtocolGuid);
    return (SHELL_NOT_FOUND);
  }

  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_HEADER), gShellDriver1HiiHandle);

  //
  // First do HII method
  //
  HandleList = GetHandleListByProtocol(&gEfiHiiConfigAccessProtocolGuid);
  for (CurrentHandle = HandleList ; CurrentHandle != NULL && *CurrentHandle != NULL && ShellStatus == SHELL_SUCCESS; CurrentHandle++){
    Found = TRUE;
    ///@todo VALIDATE
  }
  SHELL_FREE_NON_NULL(HandleList);

  //
  // Now do EFI 1.10 & UEFI 2.0 drivers
  //
  HandleList = GetHandleListByProtocol(&gEfiDriverConfigurationProtocolGuid);
  for (CurrentHandle = HandleList ; CurrentHandle != NULL && *CurrentHandle != NULL && ShellStatus == SHELL_SUCCESS; CurrentHandle++){
    Found = TRUE;
    ///@todo VALIDATE
  }

  if (!Found) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_NONE), gShellDriver1HiiHandle);
    return (SHELL_SUCCESS);
  }

  SHELL_FREE_NON_NULL(HandleList);
  return (ShellStatus);
}

/**
  Function to print out configuration information on a configurable handle.

  @param[in] DriverHandle     The driver handle to print info on.
  @param[in] ControllerHandle The controllerHandle to print on.
  @param[in] ChildrenToo      TRUE to tewst for children.
  @param[in] ProtocolMask     BIT0 for HII, BIT1 for DirverConfiguration.

  @retval SHELL_SUCCESS       The operation was successful.
**/
SHELL_STATUS
EFIAPI
PrintConfigInfoOnSingleHandle(
  IN CONST EFI_HANDLE   DriverHandle,
  IN CONST EFI_HANDLE   ControllerHandle OPTIONAL,
  IN CONST BOOLEAN      ChildrenToo,
  IN CONST UINT8        ProtocolMask // BIT0 - HII, BIT1 - DriverConfiguration
  )
{
  UINTN       Index1;
  UINTN       Index2;
  EFI_HANDLE  *ChildHandleList;
  UINTN       Count;
  UINTN       LoopVar;

  Index1 = DriverHandle     == NULL ? 0 : ConvertHandleToHandleIndex(DriverHandle    );
  Index2 = ControllerHandle == NULL ? 0 : ConvertHandleToHandleIndex(ControllerHandle);

  if ((ProtocolMask & BIT0) == BIT0) {
    ASSERT(Index1 == 0);
    ShellPrintHiiEx(
      -1, 
      -1, 
      NULL, 
      STRING_TOKEN (STR_DRVCFG_LINE_HII), 
      gShellDriver1HiiHandle, 
      Index2
      );
  }
  if ((ProtocolMask & BIT1) == BIT1) {
    PARSE_HANDLE_DATABASE_MANAGED_CHILDREN(DriverHandle, ControllerHandle, &Count, &ChildHandleList);
    for (LoopVar = 0 ; LoopVar <= Count ; LoopVar++) {
      ShellPrintHiiEx(
        -1, 
        -1, 
        NULL, 
        STRING_TOKEN (STR_DRVCFG_LINE_DRV), 
        gShellDriver1HiiHandle, 
        Index1,
        Index2,
        Count != 0 ? ChildHandleList[LoopVar] : 0
        );
    }
  }
  return (SHELL_SUCCESS);
}

/**
  Function to print out configuration information on all configurable handles.

  @param[in] ChildrenToo    TRUE to tewst for children.

  @retval SHELL_SUCCESS     The operation was successful.
**/
SHELL_STATUS
EFIAPI
PrintConfigInfoOnAll(
  IN CONST BOOLEAN ChildrenToo
  )
{
//  lcoate all the HII_CONFIG_ACCESS_PROTOCOL - those are all configurable
//  then cross reference with EFI_DRIVER_CONFIGURATION_PROTOCOL - those are legacy configurable
//  can be on chlid, but that is ok... just find the driver
  EFI_HANDLE        *HandleList;
  EFI_HANDLE        *CurrentHandle;
  EFI_HANDLE        *DriverHandleList;
  EFI_HANDLE        *ParentHandleList;
  EFI_HANDLE        *CurrentDriver;
  UINTN             Count;
  SHELL_STATUS      ShellStatus;
  EFI_STATUS        Status;
  UINTN             LoopVar;
  BOOLEAN           Found;

  Found             = FALSE;
  Count             = 0;
  HandleList        = NULL;
  CurrentHandle     = NULL;
  DriverHandleList  = NULL;
  CurrentDriver     = NULL;
  ShellStatus       = SHELL_SUCCESS;

  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_HEADER), gShellDriver1HiiHandle);
  //
  // First do HII method
  //
  HandleList = GetHandleListByProtocol(&gEfiHiiConfigAccessProtocolGuid);
  for (CurrentHandle = HandleList ; CurrentHandle != NULL && *CurrentHandle != NULL && ShellStatus == SHELL_SUCCESS; CurrentHandle++){
    // is this a driver handle itself?  if yes print options for it.
    if (!EFI_ERROR(gBS->OpenProtocol(*CurrentHandle, &gEfiDriverBindingProtocolGuid, NULL, NULL, gImageHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
      ShellStatus = PrintConfigInfoOnSingleHandle(*CurrentHandle, NULL, ChildrenToo, BIT0);
    } else {
      // get its driver and print options for it.
      Count = 0;
      Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS(*CurrentHandle, &Count, &DriverHandleList);
      if (EFI_ERROR(Status)) {
        Status = PARSE_HANDLE_DATABASE_PARENTS(*CurrentHandle, &Count, &ParentHandleList);
        if (!EFI_ERROR(Status)) {
          Status = PARSE_HANDLE_DATABASE_UEFI_DRIVERS(*ParentHandleList, &Count, &DriverHandleList);
        }
      }
      if (Count == 0) {
        Found = TRUE;
        ShellStatus = PrintConfigInfoOnSingleHandle(NULL, *CurrentHandle, ChildrenToo, BIT0);
      } else if (DriverHandleList != NULL) {
        for (LoopVar = 0 ; LoopVar < Count ; LoopVar++) {
          Found = TRUE;
          ShellStatus = PrintConfigInfoOnSingleHandle(DriverHandleList[LoopVar], *CurrentHandle, ChildrenToo, BIT0);
        }
      }
      SHELL_FREE_NON_NULL(DriverHandleList);
    }
  }
  SHELL_FREE_NON_NULL(HandleList);

  //
  // Now do EFI 1.10 & UEFI 2.0 drivers
  //
  HandleList = GetHandleListByProtocol(&gEfiDriverConfigurationProtocolGuid);
  for (CurrentHandle = HandleList ; CurrentHandle != NULL && *CurrentHandle != NULL && ShellStatus == SHELL_SUCCESS; CurrentHandle++){
    Found = TRUE;
    ShellStatus = PrintConfigInfoOnSingleHandle(*CurrentHandle, NULL, ChildrenToo, BIT1);
  }
  SHELL_FREE_NON_NULL(HandleList);
  if (!Found) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_NONE), gShellDriver1HiiHandle);
    return (SHELL_SUCCESS);
  }
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-c", TypeFlag},
  {L"-s", TypeFlag},
  {L"-v", TypeFlag},
  {L"-l", TypeValue},
  {L"-f", TypeValue},
  {L"-o", TypeValue},
  {L"-i", TypeValue},
  {NULL, TypeMax}
  };

SHELL_STATUS
EFIAPI
ShellCommandRunDrvCfg (
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

    //
    // Should be DriverHandle
    //
    Temp2 = ShellCommandLineGetRawValue(Package, 1);
    if (Temp2 == NULL) {
      //
      // no driver specified.  cannot be export, inport, or set (and no specified language)
      //
      if (ShellCommandLineGetFlag(Package, L"-s")
        ||ShellCommandLineGetFlag(Package, L"-l")
        ||ShellCommandLineGetFlag(Package, L"-o")
        ||ShellCommandLineGetFlag(Package, L"-i")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_HANDLE_REQ), gShellDriver1HiiHandle);
          ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // do a loop for validation, forcing, or printing
        //
        if (ShellCommandLineGetFlag(Package, L"-v") && ShellCommandLineGetFlag(Package, L"-f")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONF), gShellDriver1HiiHandle, L"-v", L"-f");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (ShellCommandLineGetFlag(Package, L"-v")){
          //
          // validate
          //
          ShellStatus = ValidOptionsOnAll(ShellCommandLineGetFlag(Package, L"-c"));
        } else if (ShellCommandLineGetFlag(Package, L"-f")){
          //
          // force
          //
ASSERT(FALSE);//          ShellStatus = ForceOptionsOnAll(ShellCommandLineGetFlag(Package, L"-c"));
        } else {
          //
          // display all that are configurable
          //
          ShellStatus = PrintConfigInfoOnAll(ShellCommandLineGetFlag(Package, L"-c"));
        }
      }
    } else {
      //
      // we have a driver handle, make sure it's valid then process it...
      //
      ASSERT(FALSE);
    }
  }
  ShellCommandLineFreeVarList (Package);
  SHELL_FREE_NON_NULL(Language);
  return (ShellStatus);
}
