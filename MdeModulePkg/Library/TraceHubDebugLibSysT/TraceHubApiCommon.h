/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRACE_HUB_API_COMMON_H_
#define TRACE_HUB_API_COMMON_H_

#include <Library/TraceHubDebugLib.h>

typedef enum {
  TraceHubDebugType = 0,
  TraceHubCatalogType
} TRACEHUB_PRINTTYPE;

typedef enum {
  TraceHubRoutingDisable = 0,
  TraceHubRoutingEnable,
  TraceHubRoutingMax
} TRACE_HUB_ROUTING;

typedef enum {
  TraceHubDebugLevelError = 0,
  TraceHubDebugLevelErrorWarning,
  TraceHubDebugLevelErrorWarningInfo,
  TraceHubDebugLevelErrorWarningInfoVerbose,
  TraceHubDebugLevelMax
} TRACE_HUB_DEBUG_LEVEL;

/**
  Determine whether to enable Trace Hub message.

  @param[in]  Flag            Flag to enable or disable Trace Hub message.
  @param[in]  DbgLevel        Debug Level of Trace Hub.
  @param[in]  SeverityType    Severity type of input message.

  @retval TRUE            Enable trace hub message.
  @retval FALSE           Disable trace hub message.
**/
BOOLEAN
EFIAPI
EnableTraceHubData (
  IN BOOLEAN                  Flag,
  IN UINT8                    DbgLevel,
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType
  );

/**
  Convert GUID from little endian to big endian.

  @param[in, out]  Guid   GUID in little endian format on entry. GUID in big endian format on exit.

  @retval EFI_SUCCESS      Convert GUID successfully.
**/
EFI_STATUS
EFIAPI
LittleEndianToBigEndian (
  IN OUT EFI_GUID  *Guid
  );

#endif // TRACE_HUB_API_COMMON_H_
