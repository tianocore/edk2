/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include "EnableTraceHubData.h"

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
  )
{
  UINT8    Routing;
  UINT8    DebugLevel;
  BOOLEAN  Enable;

  Routing = Verbosity & BIT0;
  if (Routing == TraceHubRoutingDisable) {
    return FALSE;
  }

  Enable     = FALSE;
  DebugLevel = (Verbosity & (BIT1 | BIT2)) >> 1;
  if (DebugLevel == TraceHubDebugLevelError) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError))) {
      Enable = TRUE;
    }
  } else if (DebugLevel == TraceHubDebugLevelErrorWarning) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError) || (SeverityType == SeverityWarning))) {
      Enable = TRUE;
    }
  } else if (DebugLevel == TraceHubDebugLevelErrorWarningInfo) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError) || (SeverityType == SeverityWarning) || (SeverityType == SeverityNormal))) {
      Enable = TRUE;
    }
  } else if (DebugLevel == TraceHubDebugLevelErrorWarningInfoVerbose) {
    Enable = TRUE;
  }

  return Enable;
}
