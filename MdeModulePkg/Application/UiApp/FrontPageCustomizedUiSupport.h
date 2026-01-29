/** @file
  This library class defines a set of interfaces to be used by customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FRONTPAGE_CUSTOMIZE_UI_SUPPORT_UI_H__
#define __FRONTPAGE_CUSTOMIZE_UI_SUPPORT_UI_H__

/**
  Create continue menu in the front page.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateContinueMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  );

/**
  Create empty line menu.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateEmptyLine (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  );

/**
  Create Select language menu in the front page with oneof opcode.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateLanguageMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  );

/**
  Create Reset menu.

  @param[in]    HiiHandle           The hii handle for the Uiapp driver.
  @param[in]    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
UiCreateResetMenu (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  );

/**
  Rename the driver name if necessary.

  @param    DriverName          Input the driver name.
  @param    NewDriverName       Return the new driver name.
  @param    EmptyLineAfter      Whether need to insert empty line.

  @retval   New driver name if compared, else NULL.

**/
typedef
BOOLEAN
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
  @param    SpecialHandlerFn    The pointer to the special handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
UiListThirdPartyDrivers (
  IN EFI_HII_HANDLE          HiiHandle,
  IN EFI_GUID                *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER  SpecialHandlerFn,
  IN VOID                    *StartOpCodeHandle
  );

/**
  This function processes the results of changes in configuration.


  @param HiiHandle       Points to the hii handle for this formset.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.
  @param Status          Return the handle status.

  @retval  TRUE          The callback successfully handled the action.
  @retval  FALSE         The callback not supported in this handler.

**/
BOOLEAN
UiSupportLibCallbackHandler (
  IN  EFI_HII_HANDLE              HiiHandle,
  IN  EFI_BROWSER_ACTION          Action,
  IN  EFI_QUESTION_ID             QuestionId,
  IN  UINT8                       Type,
  IN  EFI_IFR_TYPE_VALUE          *Value,
  OUT EFI_BROWSER_ACTION_REQUEST  *ActionRequest,
  OUT EFI_STATUS                  *Status
  );

#endif
