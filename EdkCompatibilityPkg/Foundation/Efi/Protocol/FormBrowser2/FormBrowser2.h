/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FormBrowser2.h

Abstract:

  The EFI_FORM_BROWSER2_PROTOCOL is the interface to the UEFI configuration driver.

--*/

#ifndef _FORM_BROWSER2_H_
#define _FORM_BROWSER2_H_

#include "EfiHii.h"

#define EFI_FORM_BROWSER2_PROTOCOL_GUID \
  { \
    0xb9d4c360, 0xbcfb, 0x4f9b, {0x92, 0x98, 0x53, 0xc1, 0x36, 0x98, 0x22, 0x58} \
  }

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_FORM_BROWSER2_PROTOCOL);

typedef struct {
  UINTN LeftColumn;
  UINTN RightColumn;
  UINTN TopRow;
  UINTN BottomRow;
} EFI_SCREEN_DESCRIPTOR;

typedef UINTN EFI_BROWSER_ACTION_REQUEST;

#define EFI_BROWSER_ACTION_REQUEST_NONE   0
#define EFI_BROWSER_ACTION_REQUEST_RESET  1
#define EFI_BROWSER_ACTION_REQUEST_SUBMIT 2
#define EFI_BROWSER_ACTION_REQUEST_EXIT   3

//
// The following types are currently defined:
//
typedef
EFI_STATUS
(EFIAPI *EFI_SEND_FORM2) (
  IN  CONST EFI_FORM_BROWSER2_PROTOCOL *This,
  IN  EFI_HII_HANDLE                   *Handles,
  IN  UINTN                            HandleCount,
  IN  EFI_GUID                         *FormSetGuid, OPTIONAL
  IN  UINT16                           FormId, OPTIONAL
  IN  CONST EFI_SCREEN_DESCRIPTOR      *ScreenDimensions, OPTIONAL
  OUT EFI_BROWSER_ACTION_REQUEST       *ActionRequest  OPTIONAL
  )
/*++

Routine Description:
  This is the routine which an external caller uses to direct the browser
  where to obtain it's information.

Arguments:
  This        -     A pointer to the EFI_FORM_BROWSER2_PROTOCOL instance.
  Handles     -     A pointer to an array of HII handles to display.
  HandleCount -     The number of handles in the array specified by Handle.
  FormSetGuid -     This field points to the EFI_GUID which must match the Guid field in the EFI_IFR_FORM_SET op-code for the specified
                    forms-based package.   If FormSetGuid is NULL, then this function will display the first found forms package.
  FormId      -     This field specifies which EFI_IFR_FORM to render as the first displayable page.
                    If this field has a value of 0x0000, then the forms browser will render the specified forms in their encoded order.
  ScreenDimenions - This allows the browser to be called so that it occupies a portion of the physical screen instead of
                    dynamically determining the screen dimensions.
  ActionRequest -   Points to the action recommended by the form.

Returns:
  EFI_SUCCESS           -  The function completed successfully.
  EFI_INVALID_PARAMETER -  One of the parameters has an invalid value.
  EFI_NOT_FOUND         -  No valid forms could be found to display.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_BROWSER_CALLBACK2) (
  IN CONST EFI_FORM_BROWSER2_PROTOCOL  *This,
  IN OUT UINTN                         *ResultsDataSize,
  IN OUT EFI_STRING                    ResultsData,
  IN BOOLEAN                           RetrieveData,
  IN CONST EFI_GUID                    *VariableGuid, OPTIONAL
  IN CONST CHAR16                      *VariableName  OPTIONAL
  )
/*++

Routine Description:
  This function is called by a callback handler to retrieve uncommitted state
  data from the browser.

Arguments:
  This            - A pointer to the EFI_FORM_BROWSER2_PROTOCOL instance.
  ResultsDataSize - A pointer to the size of the buffer associated with ResultsData.
                    On input, the size in bytes of ResultsData. 
                    On output, the size of data returned in ResultsData.
  ResultsData     - A string returned from an IFR browser or equivalent.
                    The results string will have no routing information in them.
  RetrieveData    - A BOOLEAN field which allows an agent to retrieve (if RetrieveData = TRUE)
                    data from the uncommitted browser state information or set
                    (if RetrieveData = FALSE) data in the uncommitted browser state information.
  VariableGuid    - An optional field to indicate the target variable GUID name to use.
  VariableName    - An optional field to indicate the target human-readable variable name.

Returns:
  EFI_SUCCESS           -  The results have been distributed or are awaiting distribution.
  EFI_BUFFER_TOO_SMALL  -  The ResultsDataSize specified was too small to contain the results data.

--*/
;

struct _EFI_FORM_BROWSER2_PROTOCOL {
  EFI_SEND_FORM2                       SendForm;
  EFI_BROWSER_CALLBACK2                BrowserCallback;
};

extern EFI_GUID gEfiFormBrowser2ProtocolGuid;

#endif
