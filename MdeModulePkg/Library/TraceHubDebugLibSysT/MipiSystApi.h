/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MIPI_SYST_API_H_
#define MIPI_SYST_API_H_

#include <Uefi.h>
#include <Library/TraceHubDebugLib.h>
#include "MipiSyst.h"

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x10.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32Ts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  );

/**
  Write 4 bytes to Trace Hub MMIO + 0x18.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32Mts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  );

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x18.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD64Mts (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT64            Data
  );

/**
  Write 1 byte to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD8 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT8             Data
  );

/**
  Write 2 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD16 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT16            Data
  );

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD32 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT32            Data
  );

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
MipiSystWriteD64 (
  IN  MIPI_SYST_HANDLE  *SystHandle,
  IN  UINT64            Data
  );

/**
  Clear data in Trace Hub MMIO addr + 0x30.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
**/
VOID
EFIAPI
MipiSystWriteFlag (
  IN  MIPI_SYST_HANDLE  *SystHandle
  );

/**
  To set operation code and descriptor for catalog or status code debug print.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  CatId           Cat id.
**/
VOID
EFIAPI
MipiSystWriteCatalogMessage (
  IN MIPI_SYST_HANDLE    *SystHandle,
  IN MIPI_SYST_SEVERITY  severity,
  IN UINT64              CatId
  );

/**
  To set operation code and descriptor for debug string print.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Type            String type.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  Len             Length of data buffer.
  @param[in]  Str             A pointer to data buffer.
**/
VOID
MipiSystWriteDebugString (
  IN        MIPI_SYST_HANDLE          *SystHandle,
  IN        MIPI_SYST_SUBTYPE_STRING  Type,
  IN        MIPI_SYST_SEVERITY        Severity,
  IN        UINT16                    Len,
  IN CONST  CHAR8                     *Str
  );

/**
  Write data to specified MMIO address according to MIPI_SYST_MSGDSC and MIPI_SYST_SCATTER_PROG.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  ScatterProg     A pointer to MIPI_SYST_SCATTER_PROG array.
  @param[in]  Pdesc           A pointer to MIPI_SYST_MSGDSC structure.
**/
VOID
EFIAPI
MipiSystScatterWrite (
  IN MIPI_SYST_HANDLE        *SystHandle,
  IN MIPI_SYST_SCATTER_PROG  *ScatterProg,
  IN CONST VOID              *Pdesc
  );

/**
  Get Epoch time.

  @retval UINT64    A numeric number for timestamp.
**/
UINT64
EFIAPI
MipiSystGetEpochUs (
  VOID
  );

/**
  Initialize or get Mipi Sys-T handle.

  @retval     MIPI_SYST_HANDLE    A pointer to MIPI_SYST_HANDLE structure.
**/
MIPI_SYST_HANDLE *
EFIAPI
GetMipiSystHandle (
  VOID
  );

/**
  Initialize MIPI_SYST_HANDLE structure.

  @param[in]  SystHandle      A pointer to MIPI_SYST_HANDLE structure.

  @retval MIPI_SYST_HANDLE    A initialized pointer to MIPI_SYST_HANDLE structure.
**/
MIPI_SYST_HANDLE *
EFIAPI
InitMipiSystHandle (
  IN MIPI_SYST_HANDLE  *MipiSystHandle
  );

/**
  Migrate data in Trace Hub UPL HOB to EfiRuntimeServicesData memory.

  @retval  EFI_SUCCESS           Migration is successful.
  @retval  EFI_ABORTED           No any Trace Hub UPL HOB instance exist.
  @retval  EFI_OUT_OF_RESOURCES  No available memory resource.

**/
EFI_STATUS
EFIAPI
MigrateTraceHubUplHobToRtMem (
  VOID
  );

/**
  Retrieve Trace Hub MMIO Address.

  @param[in]      ThUplHob          A pointer to Trace Hub UPL HOB.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS                   The process that retrieve MMIO address from Trace Hub UPL HOB is successful.
  @retval EFI_INVALID_PARAMETER         TraceAddress is NULL.
**/
EFI_STATUS
EFIAPI
GetTraceHubMmioAddress (
  IN     UINT8  *Hob,
  IN OUT UINTN  *TraceAddress
  );

/**
  Retrieve verbosity recorded in Trace Hub UPL HOB.

  @param[in]      ThUplHob       A pointer to Trace Hub UPL HOB.
  @param[in, out] Verbosity      Verbosity Value.

  @retval EFI_SUCCESS             The process that retrieve verbosity from Trace Hub UPL HOB is successful.
  @retval EFI_INVALID_PARAMETER   Verbosity is NULL.
**/
EFI_STATUS
EFIAPI
GetTraceHubVerbosity (
  IN     UINT8  *Hob,
  IN OUT UINT8  *Verbosity
  );

/**
  Get Trace Hub MMIO Address.

  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS               Set address successfully.
  @retval EFI_INVALID_PARAMETER     TraceAddress is NULL.
**/
EFI_STATUS
EFIAPI
BaseGetTraceHubMmioAddress (
  IN OUT UINTN  *TraceAddress
  );

/**
  Get Trace Hub verbosity value.

  @param[in, out] Verbosity       Verbosity Value.

  @retval EFI_SUCCESS             Get verbosity Value successfully.
  @retval EFI_INVALID_PARAMETER   Verbosity is NULL.
**/
EFI_STATUS
EFIAPI
BaseGetTraceHubVerbosity (
  IN OUT UINT8  *Verbosity
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
  Return the number of available Trace Hub debug instance.

  @retval UINT32  the number of available Trace Hub debug instance.
**/
UINT32
EFIAPI
CountDebugInstance (
  VOID
  );

#endif //MIPI_SYST_API_H_
