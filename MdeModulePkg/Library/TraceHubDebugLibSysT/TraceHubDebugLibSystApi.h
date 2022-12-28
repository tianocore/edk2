/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRACE_HUB_DEBUG_LIB_SYST_API_H_
#define TRACE_HUB_DEBUG_LIB_SYST_API_H_

#include <Uefi/UefiSpec.h>
#include "MipiWrapper.h"

typedef enum {
  TraceHubDebugType = 0,
  TraceHubCatalogType
} TRACEHUB_PRINTTYPE;

/**
  Return a initialized Mipi Sys-T handle.

  @retval     MIPI_SYST_HANDLE    A pointer to MIPI_SYST_HANDLE structure.
**/
MIPI_SYST_HANDLE *
EFIAPI
GetMipiSystHandle (
  VOID
  );

/**
  Return the number of available Trace Hub debug instance.

  @retval UINT32  the number of available Trace Hub debug instance.
**/
UINT32
EFIAPI
CountDebugInstance (
  VOID
  );

/**
  Set specified MMIO address and Verbosity to Mipi Sys-T handle.

  @param[in, out]  MipiSystHandle   A pointer to MIPI_SYST_HANDLE structure.
  @param[in, out]  DgbContext       A pointer to Trace Hub debug instance.
  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  PrintType        Either catalog print or debug print.

  @retval EFI_SUCCESS      Set specified MMIO address and Verbosity to Mipi Sys-T handle successfully.
  @retval EFI_ABORTED      No Trace Hub debug instance need to be processed.
**/
EFI_STATUS
EFIAPI
SetAttributeWithDbgContext (
  IN OUT MIPI_SYST_HANDLE         *MipiSystHandle,
  IN OUT UINT8                    *DgbContext,
  IN     TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN     TRACEHUB_PRINTTYPE       PrintType
  );

/**
  Get Trace Hub verbosity value.

  @param[in, out] Verbosity       Verbosity value.

  @retval EFI_SUCCESS             Get verbosity value successfully.
  @retval EFI_INVALID_PARAMETER   Verbosity is a NULL pointer.
**/
EFI_STATUS
EFIAPI
BaseGetTraceHubVerbosity (
  IN OUT UINT8  *Verbosity
  );

/**
  Get Trace Hub MMIO Address.

  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS               Get MMIO address successfully.
  @retval EFI_INVALID_PARAMETER     TraceAddress is a NULL pointer.
**/
EFI_STATUS
EFIAPI
BaseGetTraceHubMmioAddress (
  IN OUT UINTN  *TraceAddress
  );

/**
  Get Trace Hub verbosity value.

  @param[in]      DgbContext      A pointer to Trace Hub debug instance.
  @param[in, out] Verbosity       Verbosity value.

  @retval EFI_SUCCESS             Get verbosity value successfully.
  @retval EFI_INVALID_PARAMETER   Verbosity is a NULL pointer.
**/
EFI_STATUS
EFIAPI
GetTraceHubVerbosity (
  IN     UINT8  *DgbContext,
  IN OUT UINT8  *Verbosity
  );

/**
  Get Trace Hub MMIO Address.

  @param[in]      DgbContext        A pointer to Trace Hub debug instance.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS               Get MMIO address successfully.
  @retval EFI_INVALID_PARAMETER     TraceAddress is a NULL pointer.
**/
EFI_STATUS
EFIAPI
GetTraceHubMmioAddress (
  IN     UINT8  *DgbContext,
  IN OUT UINTN  *TraceAddress
  );

/**
  Migrate data in Trace Hub UPL HOB to EfiRuntimeServicesData memory.

  @retval  EFI_SUCCESS           Migration process is successful.
  @retval  EFI_ABORTED           No Trace Hub debug instance exist.
  @retval  EFI_OUT_OF_RESOURCES  No available memory resource.

**/
EFI_STATUS
EFIAPI
MigrateTraceHubUplHobToRtMem (
  VOID
  );

#endif // TRACE_HUB_DEBUG_LIB_SYST_API_H_
