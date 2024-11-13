/** @file
  Definition of Redfish Credential DXE driver.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2024 American Megatrends International LLC<BR>
  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_CREDENTIAL_DXE_H_
#define EDKII_REDFISH_CREDENTIAL_DXE_H_

#include <RedfishCommon.h>
#include <Protocol/EdkIIRedfishCredential2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/RedfishCredentialLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RedfishHttpLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RedfishDebugLib.h>

#define REDFISH_CREDENTIAL_DEBUG                DEBUG_VERBOSE
#define REDFISH_MANAGER_ACCOUNT_COLLECTION_URI  L"AccountService/Accounts"
#define REDFISH_URI_LENGTH                      128

///
/// Definition of REDFISH_SERVICE_LIST
///
typedef struct {
  LIST_ENTRY         NextInstance;
  REDFISH_SERVICE    RedfishService;
} REDFISH_SERVICE_LIST;

//
// Definitions of REDFISH_BOOTSTRAP_ACCOUNT_PRIVATE
//
typedef struct {
  EFI_HANDLE                            Handle;
  EFI_EVENT                             EndOfDxeEvent;
  EFI_EVENT                             ExitBootServiceEvent;
  EDKII_REDFISH_AUTH_METHOD             AuthMethod;
  CHAR8                                 *AccountName;
  EDKII_REDFISH_CREDENTIAL_PROTOCOL     RedfishCredentialProtocol;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL    RedfishCredential2Protocol;
  LIST_ENTRY                            RedfishServiceList;
} REDFISH_CREDENTIAL_PRIVATE;

#endif
