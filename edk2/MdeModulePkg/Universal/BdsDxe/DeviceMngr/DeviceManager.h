/** @file
  The platform device manager reference implement

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include "Bds.h"
#include "FrontPage.h"

//
// These are defined as the same with vfr file
//
#define DEVICE_MANAGER_FORMSET_GUID  \
  { \
  0x3ebfa8e6, 0x511d, 0x4b5b, {0xa9, 0x5f, 0xfb, 0x38, 0x26, 0xf, 0x1c, 0x27} \
  }

#define LABEL_VBIOS                          0x0040

#define DEVICE_MANAGER_FORM_ID               0x1000

#define DEVICE_KEY_OFFSET                    0x1000
#define DEVICE_MANAGER_KEY_VBIOS             0x2000

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  DeviceManagerVfrBin[];

#define DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE  EFI_SIGNATURE_32 ('D', 'M', 'C', 'B')

typedef struct {
  UINTN                           Signature;

  ///
  /// HII relative handles
  ///
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;

  ///
  /// Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;

  ///
  /// Configuration data
  ///
  UINT8                           VideoBios;
} DEVICE_MANAGER_CALLBACK_DATA;

#define DEVICE_MANAGER_CALLBACK_DATA_FROM_THIS(a) \
  CR (a, \
      DEVICE_MANAGER_CALLBACK_DATA, \
      ConfigAccess, \
      DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE \
      )
typedef struct {
  EFI_STRING_ID  StringId;
  UINT16         Class;
} DEVICE_MANAGER_MENU_ITEM;

/**
  This function is invoked if user selected a iteractive opcode from Device Manager's
  Formset. The decision by user is saved to gCallbackKey for later processing. If
  user set VBIOS, the new value is saved to EFI variable.


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
DeviceManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**

  This function registers HII packages to HII database.

  @retval EFI_SUCCESS This function complete successfully.
  @return Other value if failed to register HII packages.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  );

/**

  Call the browser and display the device manager to allow user
  to configure the platform.

  This function create the dynamic content for device manager. It includes
  section header for all class of devices, one-of opcode to set VBIOS.
  
  @retval  EFI_SUCCESS             Operation is successful.
  @retval  Other values if failed to clean up the dynamic content from HII
           database.

**/
EFI_STATUS
CallDeviceManager (
  VOID
  );

#endif
