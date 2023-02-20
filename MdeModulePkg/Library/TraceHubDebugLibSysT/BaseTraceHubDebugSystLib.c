/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TraceHubDebugLib.h>
#include <Library/MipiSysTLib.h>
#include <Library/MipiSysTLib/mipi_syst.h>
#include "TraceHubApiCommon.h"
#include "TraceHubApiInternal.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
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
  UINT32            DbgInstCount;
  UINT16            Index;

  DbgInstCount = 0;

  if ((NumberOfBytes == 0) || (Buffer == NULL)) {
    //
    // No data need to be written to Trace Hub
    //
    return EFI_ABORTED;
  }

  Status = MigrateTraceHubHobData ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CountDebugInstance (&DbgInstCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubDebugType
               );
    if (!EFI_ERROR (Status)) {
      Status = MipiSystWriteDebug (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 (UINT16)NumberOfBytes,
                 (CHAR8 *)Buffer
                 );
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

/**
  Write catalog status code message to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Id               Catalog ID.
  @param[in]  Guid             Driver Guid.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
**/
EFI_STATUS
EFIAPI
TraceHubWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN EFI_GUID                 *Guid
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  EFI_STATUS        Status;
  UINT32            DbgInstCount;
  UINT16            Index;

  DbgInstCount = 0;

  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = MigrateTraceHubHobData ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert little endian to big endian.
  //
  Status = LittleEndianToBigEndian (Guid);
  CopyMem (&MipiSystHandle.systh_guid, Guid, sizeof (EFI_GUID));
  MipiSystHandle.systh_tag.et_guid = 1;

  Status = CountDebugInstance (&DbgInstCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

/**
  Write catalog message to specified Trace Hub MMIO address.

  @param[in]  SeverityType   An error level to decide whether to enable Trace Hub data.
  @param[in]  Id             Catalog ID.
  @param[in]  NumberOfParams Number of parameters in argument list.
  @param[in]  ...            Argument list that pass to Trace Hub.

  @retval EFI_SUCCESS      Data was written to Trace Hub.
  @retval Other            Failed to output Trace Hub message.
**/
EFI_STATUS
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
  EFI_STATUS        Status;
  UINT32            DbgInstCount;

  DbgInstCount = 0;

  if (NumberOfParams > 6) {
    //
    // Message with more than 6 parameter is illegal.
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = MigrateTraceHubHobData ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle.systh_param_count = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle.systh_param[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  Status = CountDebugInstance (&DbgInstCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!EFI_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

/**
  Get Trace Hub MMIO Address.

  @param[in]      DgbContext        Always NULL.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval EFI_SUCCESS               Get MMIO address successfully.
  @retval EFI_INVALID_PARAMETER     TraceAddress is a NULL pointer.
**/
STATIC
EFI_STATUS
GetTraceHubMmioAddress (
  IN     UINT8  *DgbContext,
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
  Get visibility of Trace Hub Msg.

  @param[in]      DgbContext      Always NULL.
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
  )
{
  if ((DbgLevel == NULL) || (Flag == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *DbgLevel = FixedPcdGet8 (PcdTraceHubDebugLevel);
  *Flag     = FixedPcdGetBool (PcdEnableTraceHubDebugMsg);

  return EFI_SUCCESS;
}

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
  )
{
  if (DbgInstCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 1 for fixed PCD.
  //
  *DbgInstCount = 1;

  return EFI_SUCCESS;
}

/**
  Allocate boot time pool memory to store Trace Hub HOB data.

  @retval  EFI_SUCCESS   Migration process is successful.
  @retval  Other         Migration process is unsuccessful
**/
STATIC
EFI_STATUS
MigrateTraceHubHobData (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check whether to output Tracr Hub message.

  @param[in, out]  MipiSystHandle   A pointer to MIPI_SYST_HANDLE structure.
  @param[in, out]  DgbContext       Always NULL.
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
  )
{
  EFI_STATUS  Status;
  UINTN       Addr;
  UINT8       DbgLevel;
  BOOLEAN     Flag;

  if (MipiSystHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PrintType == TraceHubDebugType) {
    Status = GetTraceHubMsgVisibility (NULL, &Flag, &DbgLevel);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (!EnableTraceHubData (Flag, DbgLevel, SeverityType)) {
      return EFI_ABORTED;
    }
  }

  Status = GetTraceHubMmioAddress (NULL, &Addr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr = Addr;
  if (MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr == 0) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
