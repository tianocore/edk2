/** @file
  Definitions of RedfishHttpDxe

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_HTTP_DXE_H_
#define EDKII_REDFISH_HTTP_DXE_H_

#include <Uefi.h>
#include <RedfishCommon.h>
#include <IndustryStandard/Http11.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/RedfishContentCodingLib.h>
#include <Library/DebugLib.h>
#include <Library/HttpLib.h>
#include <Library/JsonLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RedfishDebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>

#include <Protocol/Http.h>
#include <Protocol/EdkIIRedfishHttpProtocol.h>
#include <Protocol/EdkIIRedfishCredential2.h>
#include <Protocol/RestEx.h>

#define REDFISH_HTTP_CACHE_LIST_SIZE      0x80
#define REDFISH_ERROR_MSG_MAX             128
#define REDFISH_DEBUG_STRING_LENGTH       200
#define REDFISH_HOST_NAME_MAX             64   // IPv6 maximum length (39) + "https://" (8) + port number (maximum 5)
#define REDFISH_HTTP_ERROR_REPORT         "Redfish HTTP %a failure(0x%x): %s"
#define REDFISH_HTTP_CACHE_DEBUG          DEBUG_MANAGEABILITY
#define REDFISH_HTTP_CACHE_DEBUG_DUMP     DEBUG_MANAGEABILITY
#define REDFISH_HTTP_CACHE_DEBUG_REQUEST  DEBUG_MANAGEABILITY

#endif
