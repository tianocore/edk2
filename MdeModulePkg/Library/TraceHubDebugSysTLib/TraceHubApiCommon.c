/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TraceHubDebugSysTLib.h>
#include "TraceHubApiCommon.h"

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
  )
{
  if (Flag == TraceHubRoutingDisable) {
    return FALSE;
  }

  if (DbgLevel == TraceHubDebugLevelError) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError))) {
      return TRUE;
    }
  } else if (DbgLevel == TraceHubDebugLevelErrorWarning) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError) || (SeverityType == SeverityWarning))) {
      return TRUE;
    }
  } else if (DbgLevel == TraceHubDebugLevelErrorWarningInfo) {
    if (((SeverityType == SeverityFatal) || (SeverityType == SeverityError) || (SeverityType == SeverityWarning) || (SeverityType == SeverityNormal))) {
      return TRUE;
    }
  } else if (DbgLevel == TraceHubDebugLevelErrorWarningInfoVerbose) {
    return TRUE;
  }

  return FALSE;
}

/**
  Convert GUID from little endian to big endian.

  @param[in, out]  Guid   GUID in little endian format on entry. GUID in big endian format on exit.

  @retval RETURN_SUCCESS      Convert GUID successfully.
**/
RETURN_STATUS
EFIAPI
LittleEndianToBigEndian (
  IN OUT GUID  *Guid
  )
{
  GUID    TempGuid;
  UINT64  GuidData4;

  ZeroMem (&TempGuid, sizeof (GUID));
  TempGuid.Data1 = SwapBytes32 (Guid->Data1);
  TempGuid.Data2 = SwapBytes16 (Guid->Data2);
  TempGuid.Data3 = SwapBytes16 (Guid->Data3);
  CopyMem (&GuidData4, Guid->Data4, sizeof (Guid->Data4));
  GuidData4 = SwapBytes64 (GuidData4);
  CopyMem (TempGuid.Data4, &GuidData4, sizeof (GuidData4));
  CopyMem (Guid, &TempGuid, sizeof (GUID));

  return RETURN_SUCCESS;
}
