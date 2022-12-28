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
#include "MipiWrapper.h"
#include "TraceHubDebugLibSystApi.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval EFI_SUCCESS      Data was written to Trace Hub or no data need to be written.
  @retval Other            Failed to pass data to Trace Hub.
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
  MIPI_SYST_HEADER  MipiSystHeader;
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

  MipiSystHandle.systh_header = &MipiSystHeader;
  InitMipiSystHandle (&MipiSystHandle);

  Status = BaseGetTraceHubMmioAddress (&Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr = (VOID *)(UINTN)Addr;
  if ( MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr == NULL) {
    return EFI_ABORTED;
  }

  WriteDebug (
    &MipiSystHandle,
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
  MIPI_SYST_HEADER  MipiSystHeader;
  EFI_GUID          ConvertedGuid;
  UINT64            GuidData4;
  UINTN             Addr;
  EFI_STATUS        Status;

  if (Guid == NULL) {
    return;
  }

  MipiSystHandle.systh_header = &MipiSystHeader;
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
  CopyMem (&MipiSystHandle.systh_guid, &ConvertedGuid, sizeof (EFI_GUID));
  MipiSystHandle.systh_tag.et_guid = 1;

  Status = BaseGetTraceHubMmioAddress (&Addr);
  if (EFI_ERROR (Status)) {
    return;
  }

  MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr = (VOID *)(UINTN)Addr;
  if (MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr == NULL) {
    return;
  }

  WriteCatalog (&MipiSystHandle, SeverityType, Id);
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
  MIPI_SYST_HEADER  MipiSystHeader;
  VA_LIST           Args;
  UINTN             Index;
  UINTN             Addr;
  EFI_STATUS        Status;

  if (NumberOfParams > 6) {
    //
    // Message with more than 6 parameter is illegal.
    //
    return;
  }

  MipiSystHandle.systh_header = &MipiSystHeader;
  InitMipiSystHandle (&MipiSystHandle);

  MipiSystHandle.systh_param_count = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle.systh_param[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  Status = BaseGetTraceHubMmioAddress (&Addr);
  if (EFI_ERROR (Status)) {
    return;
  }

  MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr = (VOID *)(UINTN)Addr;
  if (MipiSystHandle.systh_platform.TraceHubPlatformData.MmioAddr == NULL) {
    return;
  }

  WriteCatalog (&MipiSystHandle, SeverityType, Id);
}

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

  @param[in, out] Verbosity       Verbosity value.

  @retval EFI_SUCCESS             Get verbosity value successfully.
  @retval EFI_INVALID_PARAMETER   Verbosity is a NULL pointer.
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
