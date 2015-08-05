/** @file
  The platform boot manager reference implement

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_BOOT_MANAGER_H_
#define _EFI_BOOT_MANAGER_H_

#include "Ui.h"
#include "FrontPage.h"

//
// These are defined as the same with vfr file
//
#define BOOT_MANAGER_FORMSET_GUID \
  { \
  0x847bc3fe, 0xb974, 0x446d, {0x94, 0x49, 0x5a, 0xd5, 0x41, 0x2e, 0x99, 0x3b} \
  }

#define BOOT_MANAGER_FORM_ID     0x1000

#define LABEL_BOOT_OPTION        0x00
#define LABEL_BOOT_OPTION_END    0x01

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8 BootManagerVfrBin[];

#define BOOT_MANAGER_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('B', 'M', 'C', 'B')

typedef struct {
  UINTN                           Signature;

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
} BOOT_MANAGER_CALLBACK_DATA;

/**
  This call back function is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
BootManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**
  Register HII packages to HII database.

**/
VOID
InitializeBootManager  (
  VOID
  );

/**
  Enumerate possible boot options.

**/
VOID
EnumerateBootOptions  (
  VOID
  );

/**
  Remove the installed  packages from the HII Database. 

**/
VOID
FreeBootManager (
  VOID
  );

#endif
