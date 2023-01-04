/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ENABLE_TRACE_HUB_DATA_H_
#define ENABLE_TRACE_HUB_DATA_H_

#include <Library/TraceHubDebugLib.h>

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
  Determine whether to enable Trace Hub data.

  @param[in]  Verbosity       Verbosity value.
  @param[in]  SeverityType    An error level to decide whether to enable Trace Hub data.

  @retval TRUE            Enable trace hub data.
  @retval FALSE           Disable trace hub data.
**/
BOOLEAN
EFIAPI
EnableTraceHubData (
  IN UINT8                    Verbosity,
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType
  );

#endif // ENABLE_TRACE_HUB_DATA_H_
