/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRACE_HUB_API_INTERNAL_H_
#define TRACE_HUB_API_INTERNAL_H_

/**
  Collect the number of available Trace Hub debug instance.

  @param[in, out]  DbgInstCount   The number of available Trace Hub debug instance.

  @retval EFI_SUCCESS      Collect the number of available Trace Hub debug instance successfully.
  @retval Other            Failed to collect the number of available Trace Hub debug instance.
**/
STATIC
EFI_STATUS
CountDebugInstance (
  IN OUT UINT32  *DbgInstCount
  );

/**
  Check whether to output Tracr Hub message.

  @param[in, out]  MipiSystHandle   A pointer to MIPI_SYST_HANDLE structure.
  @param[in, out]  DgbContext       A pointer to Trace Hub debug instance.
  @param[in]       SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]       PrintType        Either catalog print or debug print.

  @retval EFI_SUCCESS      Current Trace Hub message need to be processed.
  @retval Other            Current Trace Hub message no need to be processed.
**/
STATIC
EFI_STATUS
CheckWhetherToOutputMsg (
  IN OUT MIPI_SYST_HANDLE         *MipiSystHandle,
  IN OUT UINT8                    *DgbContext,
  IN     TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN     TRACEHUB_PRINTTYPE       PrintType
  );

/**
  Get visibility of Trace Hub Msg.

  @param[in]      DgbContext      A pointer to Trace Hub debug instance.
  @param[in, out] Flag            Flag to enable or disable Trace Hub message.
  @param[in, out] DbgLevel        Debug Level of Trace Hub.

  @retval EFI_SUCCESS             Get visibility of Trace Hub Msg successfully.
  @retval EFI_INVALID_PARAMETER   On entry, Flag or DbgLevel is a NULL pointer.
**/
STATIC
EFI_STATUS
GetTraceHubMsgVisibility (
  IN     UINT8    *DgbContext,
  IN OUT BOOLEAN  *Flag,
  IN OUT UINT8    *DbgLevel
  );

/**
  Get Trace Hub MMIO Address.

  @param[in]      DgbContext        A pointer to Trace Hub debug instance.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS               Get MMIO address successfully.
  @retval EFI_INVALID_PARAMETER     TraceAddress is a NULL pointer.
**/
STATIC
EFI_STATUS
GetTraceHubMmioAddress (
  IN     UINT8  *DgbContext,
  IN OUT UINTN  *TraceAddress
  );

/**
  Allocate boot time pool memory to store Trace Hub HOB data.

  @retval  EFI_SUCCESS   Migration process is successful.
  @retval  Other         Migration process is unsuccessful
**/
STATIC
EFI_STATUS
MigrateTraceHubHobData (
  VOID
  );

#endif // TRACE_HUB_API_INTERNAL_H_
