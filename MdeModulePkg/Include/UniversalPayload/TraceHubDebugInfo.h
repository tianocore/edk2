/** @file
 Define the structure for the Universal Payload TraceHub Debug Info.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.9 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO_H_
#define UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO_H_

#include <UniversalPayload/UniversalPayload.h>

typedef struct {
  BOOLEAN    Flag;         // Flag to enable or disable Trace Hub debug message.
  UINT8      DebugLevel;   // Debug level for Trace Hub.
  UINTN      TraceAddress; // MMIO address where Trace Hub debug message output to.
} TRACEHUB_DEBUG_CONTEXT;

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  TRACEHUB_DEBUG_CONTEXT              DebugContext;
} UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO;

#define UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO_REVISION  1

extern EFI_GUID  gUniversalPayloadTraceHubDebugInfoGuid;

#endif // UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO_H_
