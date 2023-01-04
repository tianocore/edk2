/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TraceHubDebugLib.h>
#include <Library/BaseMemoryLib.h>
#include "EnableTraceHubData.h"
#include "MipiSyst.h"
#include "MipiSystApi.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval EFI_DEVICE_ERROR No available Mipi Sys-T handle.
  @retval Other            Data failed to write to Trace Hub.
**/
EFI_STATUS
EFIAPI
TraceHubDebugWrite (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT8                    *Buffer,
  IN UINTN                    NumberOfBytes
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  EFI_STATUS        Status;
  UINT8             Verbosity;
  UINTN             Addr;

  ASSERT (Buffer != NULL || NumberOfBytes == 0);

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return EFI_SUCCESS;
  }

  Status = BaseGetTraceHubVerbosity (&Verbosity);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!EnableTraceHubData (Verbosity, SeverityType)) {
    return EFI_ABORTED;
  }

  InitMipiSystHandle (&MipiSystHandle);

  BaseGetTraceHubMmioAddress (&Addr);
  if (Addr == 0) {
    return EFI_ABORTED;
  }

  MipiSystHandle.ThDebugMmioAddress = (VOID *)(UINTN)Addr;

  MipiSystWriteDebugString (
    &MipiSystHandle,
    MipiSystStringGeneric,
    (MIPI_SYST_SEVERITY)SeverityType,
    (UINT16)NumberOfBytes,
    (CHAR8 *)Buffer
    );

  return EFI_SUCCESS;
}

/**
  Write catalog status code message to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Id               Catalog ID.
  @param[in]  Guid             Driver Guid.
**/
VOID
EFIAPI
TraceHubWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN EFI_GUID                 *Guid
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  EFI_GUID          ConvertedGuid;
  UINT64            GuidData4;
  UINTN             Addr;

  if (Guid == NULL) {
    return;
  }

  InitMipiSystHandle (&MipiSystHandle);

  //
  // Convert little endian to big endian.
  //
  ZeroMem (&ConvertedGuid, sizeof (EFI_GUID));
  ConvertedGuid.Data1 = SwapBytes32 (Guid->Data1);
  ConvertedGuid.Data2 = SwapBytes16 (Guid->Data2);
  ConvertedGuid.Data3 = SwapBytes16 (Guid->Data3);
  CopyMem (&GuidData4, Guid->Data4, sizeof (Guid->Data4));
  GuidData4 = SwapBytes64 (GuidData4);
  CopyMem (ConvertedGuid.Data4, &GuidData4, sizeof (GuidData4));
  CopyMem (&MipiSystHandle.SystHandleGuid, &ConvertedGuid, sizeof (EFI_GUID));
  MipiSystHandle.SystHandleTag.EtGuid = 1;

  BaseGetTraceHubMmioAddress (&Addr);
  if (Addr == 0) {
    return;
  }

  MipiSystHandle.ThDebugMmioAddress = (VOID *)(UINTN)Addr;
  MipiSystWriteCatalogMessage (&MipiSystHandle, (MIPI_SYST_SEVERITY)SeverityType, Id);
}

/**
  Write catalog message to specified Trace Hub MMIO address.

  @param[in]  SeverityType   An error level to decide whether to enable Trace Hub data.
  @param[in]  Id             Catalog ID.
  @param[in]  NumberOfParams Number of parameters in argument list.
  @param[in]  ...            Argument list that pass to Trace Hub.
**/
VOID
EFIAPI
TraceHubWriteCataLog64 (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN UINTN                    NumberOfParams,
  ...
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  VA_LIST           Args;
  UINTN             Index;
  UINTN             Addr;

  if (NumberOfParams > 6) {
    //
    // Message with more than 6 parameter is illegal.
    //
    return;
  }

  InitMipiSystHandle (&MipiSystHandle);

  MipiSystHandle.SystHandleParamCount = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle.SystHandleParam[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  BaseGetTraceHubMmioAddress (&Addr);
  if (Addr == 0) {
    return;
  }

  MipiSystHandle.ThDebugMmioAddress = (VOID *)(UINTN)Addr;
  MipiSystWriteCatalogMessage (&MipiSystHandle, (MIPI_SYST_SEVERITY)SeverityType, Id);
}

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
  )
{
  if (TraceAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TraceAddress = FixedPcdGet64 (PcdTraceHubDebugAddress);

  return EFI_SUCCESS;
}

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
  )
{
  if (Verbosity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Verbosity = FixedPcdGet8 (PcdTraceHubDebugVerbosity);

  return EFI_SUCCESS;
}
