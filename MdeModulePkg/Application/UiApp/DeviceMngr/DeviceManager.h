/** @file
  The platform device manager reference implement

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include "Ui.h"
#include "FrontPage.h"
#include <Protocol/PciIo.h>

//
// These are defined as the same with vfr file
//
#define DEVICE_MANAGER_FORMSET_GUID  \
  { \
  0x3ebfa8e6, 0x511d, 0x4b5b, {0xa9, 0x5f, 0xfb, 0x38, 0x26, 0xf, 0x1c, 0x27} \
  }

#define LABEL_DEVICES_LIST                   0x1100
#define LABEL_NETWORK_DEVICE_LIST_ID         0x1101
#define LABEL_NETWORK_DEVICE_ID              0x1102
#define LABEL_END                            0xffff
#define LABEL_FORM_ID_OFFSET                 0x0100

#define LABEL_VBIOS                          0x0040

#define DEVICE_MANAGER_FORM_ID               0x1000
#define NETWORK_DEVICE_LIST_FORM_ID          0x1001
#define NETWORK_DEVICE_FORM_ID               0x1002
#define DEVICE_KEY_OFFSET                    0x4000
#define NETWORK_DEVICE_LIST_KEY_OFFSET       0x2000
#define DEVICE_MANAGER_KEY_VBIOS             0x3000
#define MAX_KEY_SECTION_LEN                  0x1000

#define QUESTION_NETWORK_DEVICE_ID           0x3FFF
//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  DeviceManagerVfrBin[];

#define DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('D', 'M', 'C', 'B')


typedef struct {
  UINTN                           Signature;

  ///
  /// Device Manager HII relative handles
  ///
  EFI_HII_HANDLE                  HiiHandle;

  EFI_HANDLE                      DriverHandle;

  ///
  /// Device Manager Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;

  ///
  /// Configuration data
  ///
  UINT8                           VideoBios;
} DEVICE_MANAGER_CALLBACK_DATA;

typedef struct {
  EFI_STRING_ID    PromptId;
  EFI_QUESTION_ID  QuestionId;
}MENU_INFO_ITEM;

typedef struct {
  UINTN           CurListLen;
  UINTN           MaxListLen;
  MENU_INFO_ITEM  *NodeList;
} MAC_ADDRESS_NODE_LIST;

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
  This function is invoked if user selected a interactive opcode from Device Manager's
  Formset. If user set VBIOS, the new value is saved to EFI variable.


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
  This  function  registers  HII  packages  to  HII  database.

**/
VOID
InitializeDeviceManager (
  VOID
  );

/**
  Remove the installed  packages from the HII Database. 

**/
VOID
FreeDeviceManager(
  VOID
  );

/**
  Dynamic create Hii information for Device Manager.

  @param   NextShowFormId     The FormId which need to be show.

**/
VOID
CreateDeviceManagerForm(
  IN EFI_FORM_ID      NextShowFormId
  );

#endif
