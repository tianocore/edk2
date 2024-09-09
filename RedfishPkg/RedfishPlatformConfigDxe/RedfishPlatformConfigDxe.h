/** @file
  This file defines the EDKII Redfish Platform Config Protocol interface.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_PLATFORM_CONFIG_DXE_H_
#define EDKII_REDFISH_PLATFORM_CONFIG_DXE_H_

#include <Uefi.h>

//
// Libraries
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/RedfishDebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

//
// Protocols
//
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/RegularExpressionProtocol.h>

//
// Produced Protocol
//
#include <Protocol/EdkIIRedfishPlatformConfig.h>

//
// Debug message in DEBUG_REDFISH_COMPONENT_PLATFORM_CONFIG_DXE scope.
// To enable the debug message for this module, below PCDs must be set.
//
// 1. DEBUG_MANAGEABILITY must be set PcdDebugPrintErrorLevel.
//
// 2  RedfishPlatformConfigDxe debug enablement must be set in
//    PcdRedfishDebugCategory (defined in RedfishPkg.dec)
//
// 3. The subordinate debug enablement for RedfishPlatformConfigDxe
//    must be set in PcdRedfishPlatformConfigDebugProperty (defined
//    in RedfishPkg.dec).
//
#define DEBUG_REDFISH_THIS_MODULE(DebugSubordinate, ...) \
  while (RedfishPlatformConfigDebugProp (DebugSubordinate)) { \
    DEBUG_REDFISH(DEBUG_REDFISH_COMPONENT_PLATFORM_CONFIG_DXE, ##__VA_ARGS__); \
    break; \
  }

#define DEBUG_REDFISH_THIS_MODULE_CODE_BEGIN(DebugSubordinate) \
  if (RedfishPlatformConfigDebugProp (DebugSubordinate)) {

#define DEBUG_REDFISH_THIS_MODULE_CODE_END()  }

#define DEBUG_REDFISH_THIS_MODULE_CODE(DebugSubordinate, Expression) \
  DEBUG_REDFISH_THIS_MODULE_CODE_BEGIN(DebugSubordinate) \
  Expression \
  DEBUG_REDFISH_THIS_MODULE_CODE_END()

// Subordinate debug property for DEBUG_REDFISH_PLATFORM_CONFIG_DXE
#define REDFISH_PLATFORM_CONFIG_DEBUG_STRING_DATABASE     0x00000001
#define REDFISH_PLATFORM_CONFIG_DEBUG_DUMP_FORMSET        0x00000002
#define REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_SEARCH  0x00000004
#define REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_REGEX   0x00000008

///
/// Definition of EDKII_REDFISH_PLATFORM_CONFIG_NOTIFY.
///
typedef struct {
  EFI_EVENT    ProtocolEvent;                  ///< Protocol notification event.
  VOID         *Registration;                  ///< Protocol notification registration.
} REDFISH_PLATFORM_CONFIG_NOTIFY;

///
/// Definition of REDFISH_PLATFORM_CONFIG_PRIVATE.
///
typedef struct {
  EFI_HANDLE                                ImageHandle;                ///< Driver image handle.
  EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL    Protocol;
  REDFISH_PLATFORM_CONFIG_NOTIFY            HiiDbNotify;
  EFI_HII_DATABASE_PROTOCOL                 *HiiDatabase;               ///< The HII database protocol.
  REDFISH_PLATFORM_CONFIG_NOTIFY            HiiStringNotify;
  EFI_HII_STRING_PROTOCOL                   *HiiString;                 ///< HII String Protocol.
  REDFISH_PLATFORM_CONFIG_NOTIFY            RegexNotify;
  EFI_REGULAR_EXPRESSION_PROTOCOL           *RegularExpressionProtocol; ///< Regular Expression Protocol.
  EFI_HANDLE                                NotifyHandle;               ///< The notify handle.
  LIST_ENTRY                                FormsetList;                ///< The list to keep cached HII formset.
  LIST_ENTRY                                PendingList;                ///< The list to keep updated HII handle.
} REDFISH_PLATFORM_CONFIG_PRIVATE;

///
/// Definition of REDFISH_STACK.
///
typedef struct {
  VOID     **Pool;
  UINTN    Size;
  UINTN    Index;
} REDFISH_STACK;

#define REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_PRIVATE, Protocol)
#define REGULAR_EXPRESSION_INCLUDE_ALL  L".*"
#define CONFIGURE_LANGUAGE_PREFIX       "x-UEFI-redfish-"
#define REDFISH_MENU_PATH_SIZE          8

// Definitions of Redfish platform config capability
#define REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH   0x000000001
#define REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED  0x000000002

/**
  Convert input unicode string to ascii string. It's caller's
  responsibility to free returned buffer using FreePool().

  @param[in]  UnicodeString     Unicode string to be converted.

  @retval CHAR8 *               Ascii string on return.

**/
CHAR8 *
StrToAsciiStr (
  IN  EFI_STRING  UnicodeString
  );

#endif
