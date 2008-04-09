/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FRONT_PAGE_H
#define _FRONT_PAGE_H

#include "DeviceMngr/DeviceManager.h"
#include "BootMaint/BootMaint.h"
#include "BootMngr/BootManager.h"
#include "String.h"

#define ONE_SECOND  10000000

//
// This is the VFR compiler generated header file which defines the
// string identifiers.
//
#define PRINTABLE_LANGUAGE_NAME_STRING_ID     0x0001

//
// These are defined as the same with vfr file
//
#define FRONT_PAGE_FORM_ID             0x1000

#define FRONT_PAGE_KEY_CONTINUE        0x1000
#define FRONT_PAGE_KEY_LANGUAGE        0x1234
#define FRONT_PAGE_KEY_BOOT_MANAGER    0x1064
#define FRONT_PAGE_KEY_DEVICE_MANAGER  0x8567
#define FRONT_PAGE_KEY_BOOT_MAINTAIN   0x9876

#define LABEL_SELECT_LANGUAGE          0x1000

#define FRONT_PAGE_FORMSET_GUID \
  { \
    0x9e0c30bc, 0x3f06, 0x4ba6, 0x82, 0x88, 0x9, 0x17, 0x9b, 0x85, 0x5d, 0xbe \
  }

#define FRONT_PAGE_CALLBACK_DATA_SIGNATURE  EFI_SIGNATURE_32 ('F', 'P', 'C', 'B')

typedef struct {
  UINTN                           Signature;

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_STRING_ID                   *LanguageToken;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} FRONT_PAGE_CALLBACK_DATA;

#define EFI_FP_CALLBACK_DATA_FROM_THIS(a) \
  CR (a, \
      FRONT_PAGE_CALLBACK_DATA, \
      ConfigAccess, \
      FRONT_PAGE_CALLBACK_DATA_SIGNATURE \
      )

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  FrontPageVfrBin[];

extern EFI_HII_DATABASE_PROTOCOL       *gHiiDatabase;
extern EFI_HII_STRING_PROTOCOL         *gHiiString;
extern EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;
extern EFI_HII_CONFIG_ROUTING_PROTOCOL *gHiiConfigRouting;

extern UINTN    gCallbackKey;
extern BOOLEAN  gConnectAllHappened;

EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

EFI_STATUS
EFIAPI
FrontPageCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

EFI_STATUS
InitializeFrontPage (
  IN BOOLEAN    ReInitializeStrings
  );

EFI_STATUS
GetProducerString (
  IN      EFI_GUID                  *ProducerGuid,
  IN      EFI_STRING_ID             Token,
  OUT     CHAR16                    **String
  );

BOOLEAN
TimeCompare (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  );

VOID
PlatformBdsEnterFrontPage (
  IN UINT16                 TimeoutDefault,
  IN BOOLEAN                ConnectAllHappened
  );

#endif // _FRONT_PAGE_H_

