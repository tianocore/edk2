/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiConfig.h

Abstract:


--*/

#ifndef _ISCSI_CONFIG_H_
#define _ISCSI_CONFIG_H_

//#include "Tiano.h"
//#include "EfiDriverLib.h"
//#include "Base.h"
#include <Library/HiiLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
//#include "EfiPrintLib.h"
//#include EFI_PROTOCOL_DEFINITION (Hii)
//#include EFI_PROTOCOL_DEFINITION (FormBrowser)
//#include EFI_PROTOCOL_DEFINITION (FormCallback)

#include <Library/NetLib.h>
#include "IScsiConfigNVDataStruc.h"

extern UINT8  IScsiConfigDxeBin[];
extern UINT8  iSCSIStrings[];

#define ISCSI_INITATOR_NAME_VAR_NAME        L"I_NAME"

#define ISCSI_CONFIG_VAR_ATTR               (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define ISCSI_FORM_CALLBACK_INFO_SIGNATURE  EFI_SIGNATURE_32 ('I', 'f', 'c', 'i')

#define ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK(Callback) \
  CR ( \
  Callback, \
  ISCSI_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  ISCSI_FORM_CALLBACK_INFO_SIGNATURE \
  )

#pragma pack(1)

typedef struct _ISCSI_MAC_INFO {
  EFI_MAC_ADDRESS Mac;
  UINT8           Len;
} ISCSI_MAC_INFO;

typedef struct _ISCSI_DEVICE_LIST {
  UINT8           NumDevice;
  ISCSI_MAC_INFO  MacInfo[1];
} ISCSI_DEVICE_LIST;

#pragma pack()

typedef struct _ISCSI_CONFIG_FORM_ENTRY {
  NET_LIST_ENTRY                Link;
  EFI_HANDLE                    Controller;
  CHAR16                        MacString[95];
  STRING_REF                    PortTitleToken;
  STRING_REF                    PortTitleHelpToken;

  ISCSI_SESSION_CONFIG_NVDATA   SessionConfigData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA AuthConfigData;
} ISCSI_CONFIG_FORM_ENTRY;

typedef struct _ISCSI_FORM_CALLBACK_INFO {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  EFI_HII_DATABASE_PROTOCOL        *HiiDatabase;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  UINT16                           *KeyList;
  VOID                             *FormBuffer;
  EFI_HII_HANDLE                   RegisteredHandle;
  ISCSI_CONFIG_FORM_ENTRY          *Current;
} ISCSI_FORM_CALLBACK_INFO;

EFI_STATUS
IScsiConfigUpdateForm (
  IN EFI_HANDLE  DriverBindingHandle,
  IN EFI_HANDLE  Controller,
  IN BOOLEAN     AddForm
  );

EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  );

EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  );

#endif
