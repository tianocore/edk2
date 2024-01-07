/** @file
  This file defines the EDKII Redfish Platform Config Protocol interface.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

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
#include <Library/PrintLib.h>
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
#define REGULAR_EXPRESSION_INCLUDE_ALL   L".*"
#define CONFIGURE_LANGUAGE_PREFIX        "x-uefi-redfish-"
#define REDFISH_PLATFORM_CONFIG_VERSION  0x00010000
#define REDFISH_PLATFORM_CONFIG_DEBUG    DEBUG_MANAGEABILITY
#define REDFISH_MENU_PATH_SIZE           8

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
