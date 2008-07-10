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

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;

  //
  // Configuration data
  //
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
  EDES_TODO: Add function description.

  @param This            EDES_TODO: Add parameter description
  @param Action          EDES_TODO: Add parameter description
  @param QuestionId      EDES_TODO: Add parameter description
  @param Type            EDES_TODO: Add parameter description
  @param Value           EDES_TODO: Add parameter description
  @param ActionRequest   EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

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
  )
;

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  )
;

/**
  EDES_TODO: Add function description.

  @param VOID            EDES_TODO: Add parameter description

  @return EDES_TODO: Add description for return value

**/
EFI_STATUS
CallDeviceManager (
  VOID
  )
;

#endif
