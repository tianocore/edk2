/** @file

  This library class defines a set of interfaces to customize Ui module

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Protocol/HiiConfigAccess.h>
#include "BootMaintenanceManagerCustomizedUiSupport.h"

/**
  Customize menus in the page.

  @param[in]  HiiHandle             The HII Handle of the form to update.
  @param[in]  StartOpCodeHandle     The context used to insert opcode.
  @param[in]  CustomizePageType     The page type need to be customized.

**/
VOID
UiCustomizeBMMPage (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
  )
{
  //
  // Create "Boot Option" menu.
  //
  BmmCreateBootOptionMenu(HiiHandle, StartOpCodeHandle);
  //
  // Create "Driver Option" menu.
  //
  BmmCreateDriverOptionMenu(HiiHandle, StartOpCodeHandle);
  //
  // Create "Com Option" menu.
  //
  BmmCreateComOptionMenu(HiiHandle, StartOpCodeHandle);
  //
  // Create "Boot From File" menu.
  //
  BmmCreateBootFromFileMenu(HiiHandle, StartOpCodeHandle);

  //
  // Find third party drivers which need to be shown in the Bmm page.
  //
  BmmListThirdPartyDrivers (HiiHandle, &gEfiIfrBootMaintenanceGuid, NULL, StartOpCodeHandle);

  //
  // Create empty line.
  //
  BmmCreateEmptyLine (HiiHandle, StartOpCodeHandle);

  //
  // Create "Boot Next" menu.
  //
  BmmCreateBootNextMenu (HiiHandle, StartOpCodeHandle);
  //
  // Create "Time Out" menu.
  //
  BmmCreateTimeOutMenu (HiiHandle, StartOpCodeHandle);
}

/**
  This function processes the results of changes in configuration.


  @param HiiHandle       Points to the hii handle for this formset.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
UiBMMCallbackHandler (
  IN  EFI_HII_HANDLE                         HiiHandle,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  return EFI_UNSUPPORTED;
}
