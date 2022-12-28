/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi/UefiBaseType.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TraceHubDebugLib.h>
#include <UniversalPayload/TraceHubDebugInfo.h>
#include "EnableTraceHubData.h"
#include "MipiWrapper.h"
#include "TraceHubDebugLibSystApi.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval EFI_SUCCESS      Data was written to Trace Hub or no data need to be written.
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
  UINT8             *DgbContext;
  UINTN             Index;
  UINTN             DbgInstCount;

  ASSERT (Buffer != NULL || NumberOfBytes == 0);

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return EFI_SUCCESS;
  }

  MipiSystHandle.systh_header = &MipiSystHeader;
  InitMipiSystHandle (&MipiSystHandle);

  DbgInstCount = CountDebugInstance ();

  DgbContext = (UINT8 *)GetFirstGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid);
  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               &MipiSystHandle,
               DgbContext,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubDebugType
               );
    if (!EFI_ERROR (Status)) {
      WriteDebug (
        &MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        (UINT16)NumberOfBytes,
        (CHAR8 *)Buffer
        );
      if (DgbContext != NULL) {
        DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
      }
    }
  }

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
  UINTN             DbgInstCount;
  UINT8             *DgbContext;
  EFI_STATUS        Status;
  UINTN             Index;

  MipiSystHandle.systh_header = &MipiSystHeader;
  InitMipiSystHandle (&MipiSystHandle);

  if (Guid != NULL) {
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
  } else {
    MipiSystHandle.systh_tag.et_modunit = 2;
    MipiSystHandle.systh_tag.et_guid    = 0;
  }

  DbgInstCount = CountDebugInstance ();

  DgbContext = (UINT8 *)GetFirstGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid);
  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               &MipiSystHandle,
               DgbContext,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      WriteCatalog (
        &MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        Id
        );
      if (DgbContext != NULL) {
        DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
      }
    }
  }
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
  UINTN             DbgInstCount;
  UINT8             *DgbContext;
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

  DbgInstCount = CountDebugInstance ();

  DgbContext = (UINT8 *)GetFirstGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid);
  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               &MipiSystHandle,
               DgbContext,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      WriteCatalog (
        &MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        Id
        );
      if (DgbContext != NULL) {
        DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
      }
    }
  }
}

/**
  Return the number of available Trace Hub debug instance.

  @retval UINT32  the number of available Trace Hub debug instance.
**/
UINT32
EFIAPI
CountDebugInstance (
  VOID
  )
{
  UINT32  DbgInstCount;
  UINT8   *DgbContext;

  DbgInstCount = 0;
  DgbContext   = (UINT8 *)GetFirstGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid);
  if (DgbContext != NULL) {
    while (DgbContext != NULL) {
      DbgInstCount++;
      DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
    }
  } else {
    DbgInstCount++;
  }

  return DbgInstCount;
}

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
  )
{
  EFI_STATUS  Status;
  UINT8       Verbosity;
  UINTN       Addr;

  if (PrintType == TraceHubDebugType) {
    Status = GetTraceHubVerbosity (DgbContext, &Verbosity);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!EnableTraceHubData (Verbosity, SeverityType)) {
      if (DgbContext != NULL) {
        DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
        return EFI_ABORTED;
      } else {
        return EFI_ABORTED;
      }
    }
  }

  Status = GetTraceHubMmioAddress (DgbContext, &Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr = (VOID *)(UINTN)Addr;
  if (MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr == NULL) {
    if (DgbContext != NULL) {
      DgbContext = (UINT8 *)GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
      return EFI_ABORTED;
    } else {
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

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
  )
{
  UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO  *ThDebugInfo;

  if (TraceAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DgbContext != NULL) {
    ThDebugInfo   = GET_GUID_HOB_DATA (DgbContext);
    *TraceAddress = ThDebugInfo->DebugContext.TraceAddress;
  } else {
    *TraceAddress = FixedPcdGet64 (PcdTraceHubDebugAddress);
  }

  return EFI_SUCCESS;
}

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
  )
{
  UNIVERSAL_PAYLOAD_TRACEHUB_DEBUG_INFO  *ThDebugInfo;

  if (Verbosity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DgbContext != NULL) {
    ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
    *Verbosity  = ThDebugInfo->DebugContext.TraceVerbosity;
  } else {
    *Verbosity = FixedPcdGet8 (PcdTraceHubDebugVerbosity);
  }

  return EFI_SUCCESS;
}
