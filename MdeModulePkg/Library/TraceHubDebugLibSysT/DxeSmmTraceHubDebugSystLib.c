/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TraceHubDebugLib.h>
#include <Library/DebugLib.h>
#include <UniversalPayload/TraceHubDebugInfo.h>
#include "EnableTraceHubData.h"
#include "MipiWrapper.h"
#include "TraceHubDebugLibSystApi.h"

TRACEHUB_DEBUG_CONTEXT  *mThDebugContextArray = NULL;
MIPI_SYST_HANDLE        *mMipiSystHandle      = NULL;
UINT32                  mDbgInstCount         = 0;

/**
  Return a initialized Mipi Sys-T handle.

  @retval     MIPI_SYST_HANDLE    A pointer to MIPI_SYST_HANDLE structure.
**/
MIPI_SYST_HANDLE *
EFIAPI
GetMipiSystHandle (
  VOID
  )
{
  MIPI_SYST_HANDLE  *MipiSystHandle;

  MipiSystHandle = NULL;

  if (mMipiSystHandle == NULL) {
    MipiSystHandle = AllocateRuntimePool (sizeof (MIPI_SYST_HANDLE) + sizeof (MIPI_SYST_HEADER));
    if (MipiSystHandle == NULL) {
      return NULL;
    }

    MipiSystHandle->systh_header = (MIPI_SYST_HEADER *)((UINT8 *)MipiSystHandle + sizeof (MIPI_SYST_HANDLE));
    InitMipiSystHandle (MipiSystHandle);
    mMipiSystHandle = MipiSystHandle;
  } else {
    ZeroMem (&mMipiSystHandle->systh_tag, sizeof (MIPI_SYST_MSG_TAG));
    ZeroMem (mMipiSystHandle->systh_param, sizeof (mMipiSystHandle->systh_param));
    ZeroMem (&mMipiSystHandle->systh_param_count, sizeof (mMipiSystHandle->systh_param_count));
    ZeroMem (&mMipiSystHandle->systh_guid, sizeof (EFI_GUID));
    ZeroMem (&mMipiSystHandle->systh_flags, sizeof (MIPI_SYST_HANDLE_FLAGS));
    ZeroMem (&mMipiSystHandle->systh_platform, sizeof (mMipiSystHandle->systh_platform));
  }

  return mMipiSystHandle;
}

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
  MIPI_SYST_HANDLE  *MipiSystHandle;
  EFI_STATUS        Status;
  UINT16            Index;

  ASSERT (Buffer != NULL || NumberOfBytes == 0);

  Status = EFI_SUCCESS;

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return EFI_SUCCESS;
  }

  Status = MigrateTraceHubUplHobToRtMem ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle = GetMipiSystHandle ();
  if (MipiSystHandle == NULL) {
    return EFI_ABORTED;
  }

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               MipiSystHandle,
               (UINT8 *)&mThDebugContextArray[Index],
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubDebugType
               );
    if (!EFI_ERROR (Status)) {
      WriteDebug (
        MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        (UINT16)NumberOfBytes,
        (CHAR8 *)Buffer
        );
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
  MIPI_SYST_HANDLE  *MipiSystHandle;
  EFI_GUID          ConvertedGuid;
  UINT64            GuidData4;
  UINTN             Index;
  EFI_STATUS        Status;

  Status = MigrateTraceHubUplHobToRtMem ();
  if (EFI_ERROR (Status)) {
    return;
  }

  MipiSystHandle = GetMipiSystHandle ();
  if (MipiSystHandle == NULL) {
    return;
  }

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
    CopyMem (&MipiSystHandle->systh_guid, &ConvertedGuid, sizeof (EFI_GUID));
    MipiSystHandle->systh_tag.et_guid = 1;
  } else {
    MipiSystHandle->systh_tag.et_modunit = 2;
    MipiSystHandle->systh_tag.et_guid    = 0;
  }

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               MipiSystHandle,
               (UINT8 *)&mThDebugContextArray[Index],
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      WriteCatalog (
        MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        Id
        );
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
  MIPI_SYST_HANDLE  *MipiSystHandle;
  VA_LIST           Args;
  UINTN             Index;
  EFI_STATUS        Status;

  if (NumberOfParams > 6) {
    //
    // Message with more than 6 parameter is illegal.
    //
    return;
  }

  Status = MigrateTraceHubUplHobToRtMem ();
  if (EFI_ERROR (Status)) {
    return;
  }

  MipiSystHandle = GetMipiSystHandle ();
  if (MipiSystHandle == NULL) {
    return;
  }

  MipiSystHandle->systh_param_count = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle->systh_param[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = SetAttributeWithDbgContext (
               MipiSystHandle,
               (UINT8 *)&mThDebugContextArray[Index],
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      WriteCatalog (
        MipiSystHandle,
        (MIPI_SYST_SEVERITY)SeverityType,
        Id
        );
    }
  }
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

  ThDebugInfo   = GET_GUID_HOB_DATA (DgbContext);
  *TraceAddress = ThDebugInfo->DebugContext.TraceAddress;

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

  ThDebugInfo = GET_GUID_HOB_DATA (DgbContext);
  *Verbosity  = ThDebugInfo->DebugContext.TraceVerbosity;

  return EFI_SUCCESS;
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
  TRACEHUB_DEBUG_CONTEXT  *ThDbgContext;
  UINT8                   Verbosity;

  ThDbgContext = (TRACEHUB_DEBUG_CONTEXT *)DgbContext;
  Verbosity    = ThDbgContext->TraceVerbosity;

  if (PrintType == TraceHubDebugType) {
    if (!EnableTraceHubData (Verbosity, SeverityType)) {
      return EFI_ABORTED;
    }
  }

  MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr = (VOID *)(UINTN)(ThDbgContext->TraceAddress);
  if (MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr == NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

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
  )
{
  UINT8   *DgbContext;
  UINT16  Index;
  UINT8   Verbosity;
  UINTN   TraceAddress;

  Index        = 0;
  DgbContext   = NULL;
  Verbosity    = 0;
  TraceAddress = 0;

  if (mThDebugContextArray == NULL) {
    mDbgInstCount = CountDebugInstance ();
    if (mDbgInstCount == 0) {
      return EFI_ABORTED;
    }

    mThDebugContextArray = AllocateRuntimeZeroPool (sizeof (TRACEHUB_DEBUG_CONTEXT) * mDbgInstCount);
    if (mThDebugContextArray == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DgbContext = GetFirstGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid);
    for (Index = 0; Index < mDbgInstCount; Index++) {
      GetTraceHubMmioAddress (DgbContext, &TraceAddress);
      mThDebugContextArray[Index].TraceAddress = TraceAddress;
      GetTraceHubVerbosity (DgbContext, &Verbosity);
      mThDebugContextArray[Index].TraceVerbosity = Verbosity;
      DgbContext                                 = GetNextGuidHob (&gUniversalPayloadTraceHubDebugInfoGuid, GET_NEXT_HOB (DgbContext));
    }
  }

  return EFI_SUCCESS;
}
