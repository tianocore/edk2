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

#ifndef __FRONTPAGE_CUSTOMIZED_UI_H__
#define __FRONTPAGE_CUSTOMIZED_UI_H__

/**
  Update the banner string in the front page.

  Current layout for the banner string like below:
  PS: Totally only 5 lines of banner supported.

  Line 1: Left BannerStr                           RightBannerStr
  Line 2: Left BannerStr                           RightBannerStr
  Line 3: Left BannerStr                           RightBannerStr
  Line 4: Left BannerStr                           RightBannerStr
  Line 5: Left BannerStr                           RightBannerStr
  <EmptyLine>
  First menu in front page.
  ...

  @param  LineIndex         The line index of the banner need to check.
  @param  LeftOrRight       The left or right banner need to check.
  @param  BannerStr         Banner string need to update.
                            Input the current string and user can update
                            it and return the new string.

**/
VOID
UiCustomizeFrontPageBanner (
  IN     UINTN          LineIndex,
  IN     BOOLEAN        LeftOrRight,
  IN OUT EFI_STRING     *BannerStr
  );

/**
  Customize menus in the page.

  @param[in]  HiiHandle             The HII Handle of the form to update.
  @param[in]  StartOpCodeHandle     The context used to insert opcode.

**/
VOID
UiCustomizeFrontPage (
  IN EFI_HII_HANDLE  HiiHandle,
  IN VOID            *StartOpCodeHandle
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

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.

**/
EFI_STATUS
UiFrontPageCallbackHandler (
  IN  EFI_HII_HANDLE                         HiiHandle,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

#endif
