/**@file
Framework to UEFI 2.1 Setup Browser Thunk. The file consume EFI_FORM_BROWSER2_PROTOCOL
to produce a EFI_FORM_BROWSER_PROTOCOL.

Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"

/**
  This is the Framework Setup Browser interface which displays a FormSet.

  @param This           The EFI_FORM_BROWSER_PROTOCOL context.
  @param UseDatabase    TRUE if the FormSet is from HII database. The Thunk implementation
                        only support UseDatabase is TRUE.
  @param Handle         The Handle buffer.
  @param HandleCount    The number of Handle in the Handle Buffer. It must be 1 for this implementation.
  @param Packet         The pointer to data buffer containing IFR and String package. Not supported.
  @param CallbackHandle Not supported.
  @param NvMapOverride  The buffer is used only when there is no NV variable to define the 
                        current settings and the caller needs to provide to the browser the
                        current settings for the the "fake" NV variable. If used, no saving of
                        an NV variable is possbile. This parameter is also ignored if Handle is NULL.

  @retval EFI_SUCCESS             If the Formset is displayed correctly.
  @retval EFI_UNSUPPORTED         If UseDatabase is FALSE or HandleCount is not 1.
  @retval EFI_INVALID_PARAMETER   If the *Handle passed in is not found in the database.
**/

EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL         *This,
  IN  BOOLEAN                           UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE          *Handle,
  IN  UINTN                             HandleCount,
  IN  FRAMEWORK_EFI_IFR_PACKET          *Packet, OPTIONAL
  IN  EFI_HANDLE                        CallbackHandle, OPTIONAL
  IN  UINT8                             *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR   *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                           *ResetRequired OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_BROWSER_ACTION_REQUEST        ActionRequest;
  HII_THUNK_CONTEXT                 *ThunkContext;
  HII_THUNK_PRIVATE_DATA            *Private;
  EFI_FORMBROWSER_THUNK_PRIVATE_DATA *BrowserPrivate;

  if (!UseDatabase) {
    //
    // ThunkSendForm only support displays forms registered into the HII database.
    //
    return EFI_UNSUPPORTED;
  }

  if (HandleCount != 1 ) {
    return EFI_UNSUPPORTED;
  }

  BrowserPrivate = EFI_FORMBROWSER_THUNK_PRIVATE_DATA_FROM_THIS (This);
  Private = BrowserPrivate->ThunkPrivate;

  ThunkContext = FwHiiHandleToThunkContext (Private, *Handle);
  if (ThunkContext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (NvMapOverride != NULL) {
    ThunkContext->NvMapOverride = NvMapOverride;
  }

  Status = mFormBrowser2Protocol->SendForm (
                                    mFormBrowser2Protocol,
                                    &ThunkContext->UefiHiiHandle,
                                    1,
                                    NULL,
                                    0,
                                    (EFI_SCREEN_DESCRIPTOR *) ScreenDimensions,
                                    &ActionRequest
                                    );

  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    *ResetRequired = TRUE;
  }
  
  return Status;
}

/** 

  Rountine used to display a generic dialog interface and return 
  the Key or Input from user input.

  @param NumberOfLines The number of lines for the dialog box.
  @param HotKey        Defines if a single character is parsed (TRUE) and returned in KeyValue
                       or if a string is returned in StringBuffer.
  @param MaximumStringSize The maximum size in bytes of a typed-in string.
  @param StringBuffer  On return contains the typed-in string if HotKey
         is FALSE.
  @param KeyValue      The EFI_INPUT_KEY value returned if HotKey is TRUE.
  @param String        The pointer to the first string in the list of strings
                       that comprise the dialog box.
  @param ...           A series of NumberOfLines text strings that will be used
                       to construct the dialog box.
  @retval EFI_SUCCESS  The dialog is created successfully and user interaction was received.
  @retval EFI_DEVICE_ERROR The user typed in an ESC.
  @retval EFI_INVALID_PARAMETER One of the parameters was invalid.(StringBuffer == NULL && HotKey == FALSE).
**/
EFI_STATUS
EFIAPI 
ThunkCreatePopUp (
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *KeyValue,
  IN  CHAR16                          *String,
  ...
  )
{
  EFI_STATUS                        Status;
  VA_LIST                           Marker;

  if (HotKey != TRUE) {
    return EFI_UNSUPPORTED;
  }

  VA_START (Marker, KeyValue);
  
  Status = IfrLibCreatePopUp2 (NumberOfLines, KeyValue, Marker);

  VA_END (Marker);
  
  return Status;
}

