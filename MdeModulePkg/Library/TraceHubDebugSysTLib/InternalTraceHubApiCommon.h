/** @file
This header file declares functions and type for common use.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_TRACE_HUB_API_COMMON_H_
#define INTERNAL_TRACE_HUB_API_COMMON_H_

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
  );

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
  );

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
  );

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
  );

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
  );

#endif // INTERNAL_TRACE_HUB_API_COMMON_H_
