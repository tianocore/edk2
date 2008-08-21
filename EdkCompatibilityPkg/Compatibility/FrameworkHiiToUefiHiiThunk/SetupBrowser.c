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

