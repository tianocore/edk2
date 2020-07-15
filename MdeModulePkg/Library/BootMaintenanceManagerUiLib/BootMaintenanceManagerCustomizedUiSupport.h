/** @file
  This library class defines a set of interfaces to be used by customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BOOT_MAINTENANCE_MANAGER_UI_LIB_H__
#define __BOOT_MAINTENANCE_MANAGER_UI_LIB_H__

/**
  Create Time Out Menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateTimeOutMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create the dynamic item to allow user to set the "BootNext" vaule.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootNextMenu(
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create Boot Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootOptionMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create Driver Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateDriverOptionMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create Com Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateComOptionMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create Com Option menu in the page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateBootFromFileMenu (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
BmmCreateEmptyLine (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  );

/**
  Rename the driver name if necessary.

  @param    DriverName          Input the driver name.
  @param    NewDriverName       Return the new driver name.
  @param    EmptyLineAfter      Whether need to insert empty line.

  @retval   New driver name if compared, else NULL.

**/
typedef
EFI_STATUS
(EFIAPI *DRIVER_SPECIAL_HANDLER)(
  IN  CHAR16                   *DriverName,
  OUT CHAR16                   **NewName,
  OUT BOOLEAN                  *EmptyLineAfter
);

/**
  Search the drivers in the system which need to show in the front page
  and insert the menu to the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    ClassGuid           The class guid for the driver which is the target.
  @param    SpecialHandlerFn      The pointer to the specail handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
BmmListThirdPartyDrivers (
  IN EFI_HII_HANDLE              HiiHandle,
  IN EFI_GUID                    *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER      SpecialHandlerFn,
  IN VOID                        *StartOpCodeHandle
  );

#endif
