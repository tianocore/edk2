/** @file
Functions implementation defined in this file are common for all type of TraceHubDebugSysTLib

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TraceHubDebugSysTLib.h>
#include <Library/MipiSysTLib/mipi_syst.h>
#include <Guid/TraceHubDebugInfoHob.h>
#include "InternalTraceHubApiCommon.h"
#include "InternalTraceHubApi.h"

/**
  Conditionally determine whether to enable Trace Hub message.

  @param[in]  Flag            Flag to enable or disable Trace Hub message.
  @param[in]  DbgLevel        Debug Level of Trace Hub.
  @param[in]  SeverityType    Severity type of input message.

  @retval TRUE            Enable trace hub message.
  @retval FALSE           Disable trace hub message.
**/
BOOLEAN
EFIAPI
TraceHubDataEnabled (
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
  Convert GUID from LE to BE or BE to LE.

  @param[in]  Guid           GUID that need to be converted.
  @param[out] ConvertedGuid  GUID that is converted.
**/
VOID
EFIAPI
SwapBytesGuid (
  IN  GUID  *Guid,
  OUT GUID  *ConvertedGuid
  )
{
  CopyGuid (ConvertedGuid, Guid);
  ConvertedGuid->Data1 = SwapBytes32 (ConvertedGuid->Data1);
  ConvertedGuid->Data2 = SwapBytes16 (ConvertedGuid->Data2);
  ConvertedGuid->Data3 = SwapBytes16 (ConvertedGuid->Data3);
}

/**
  Check whether to output Trace Hub message according to some conditions.
  Trace Hub message will be disabled if TraceHubDataEnabled() return FALSE
  or Trace Hub MMIO address is 0.

  @param[in, out]  MipiSystHandle   A pointer to MIPI_SYST_HANDLE structure.
  @param[in]       DbgContext       A pointer to Trace Hub debug instance.
  @param[in]       SeverityType     Severity type of input message.
  @param[in]       PrintType        Either catalog print or debug print.

  @retval RETURN_SUCCESS      Current Trace Hub message need to be output.
  @retval Other               Current Trace Hub message will be disabled.
**/
RETURN_STATUS
EFIAPI
CheckWhetherToOutputMsg (
  IN OUT MIPI_SYST_HANDLE         *MipiSystHandle,
  IN     UINT8                    *DbgContext,
  IN     TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN     TRACEHUB_PRINTTYPE       PrintType
  )
{
  UINT8          DbgLevel;
  BOOLEAN        Flag;
  UINT64         Addr;
  RETURN_STATUS  Status;

  if (MipiSystHandle == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (PrintType == TraceHubDebugType) {
    Status = GetTraceHubMsgVisibility (DbgContext, &Flag, &DbgLevel);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    if (!TraceHubDataEnabled (Flag, DbgLevel, SeverityType)) {
      return RETURN_ABORTED;
    }
  }

  Status = GetTraceHubMmioAddress (DbgContext, &Addr);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr = Addr;
  if (MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr == 0) {
    return RETURN_ABORTED;
  }

  return RETURN_SUCCESS;
}

/**
  Get Trace Hub MMIO Address.

  @param[in]      DbgContext        A pointer to Trace Hub debug instance.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval RETURN_SUCCESS      Operation is successfully.
  @retval Other               Operation is failed.
**/
RETURN_STATUS
EFIAPI
GetTraceHubMmioAddress (
  IN     UINT8   *DbgContext,
  IN OUT UINT64  *TraceAddress
  )
{
  TRACEHUB_DEBUG_INFO_HOB  *ThDbgContext;

  if (TraceAddress == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (DbgContext != NULL) {
    ThDbgContext  = (TRACEHUB_DEBUG_INFO_HOB *)DbgContext;
    *TraceAddress = ThDbgContext->TraceHubMmioAddress;
  } else {
    *TraceAddress = FixedPcdGet64 (PcdTraceHubDebugMmioAddress);
  }

  return RETURN_SUCCESS;
}

/**
  Get visibility of Trace Hub Msg.

  @param[in]      DbgContext      A pointer to Trace Hub debug instance.
  @param[in, out] Flag            Flag to enable or disable Trace Hub message.
  @param[in, out] DbgLevel        Debug Level of Trace Hub.

  @retval RETURN_SUCCESS      Operation is successfully.
  @retval Other               Operation is failed.
**/
RETURN_STATUS
EFIAPI
GetTraceHubMsgVisibility (
  IN     UINT8    *DbgContext,
  IN OUT BOOLEAN  *Flag,
  IN OUT UINT8    *DbgLevel
  )
{
  TRACEHUB_DEBUG_INFO_HOB  *ThDbgContext;

  if ((Flag == NULL) || (DbgLevel == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (DbgContext != NULL) {
    ThDbgContext = (TRACEHUB_DEBUG_INFO_HOB *)DbgContext;
    *Flag        = ThDbgContext->Flag;
    *DbgLevel    = ThDbgContext->DebugLevel;
  } else {
    *DbgLevel = FixedPcdGet8 (PcdTraceHubDebugLevel);
    *Flag     = FixedPcdGetBool (PcdEnableTraceHubDebugMsg);
  }

  return RETURN_SUCCESS;
}
