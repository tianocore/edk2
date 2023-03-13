/** @file

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TraceHubDebugSysTLib.h>
#include <Library/MipiSysTLib.h>
#include <Library/MipiSysTLib/mipi_syst.h>
#include "TraceHubApiCommon.h"
#include "TraceHubApiInternal.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    Number of bytes to be written.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubDebugWrite (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT8                    *Buffer,
  IN UINTN                    NumberOfBytes
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  RETURN_STATUS     Status;
  UINT32            DbgInstCount;
  UINT16            Index;

  DbgInstCount = 0;

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return RETURN_ABORTED;
  }

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Status = MigrateTraceHubHobData ();

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = CountDebugInstance (&DbgInstCount);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubDebugType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteDebug (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 (UINT16)NumberOfBytes,
                 (CHAR8 *)Buffer
                 );
      if (RETURN_ERROR (Status)) {
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

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN GUID                     *Guid
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  RETURN_STATUS     Status;
  UINT32            DbgInstCount;
  UINT16            Index;

  DbgInstCount = 0;

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Status = MigrateTraceHubHobData ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Convert little endian to big endian.
  //
  Status = LittleEndianToBigEndian (Guid);
  CopyMem (&MipiSystHandle.systh_guid, Guid, sizeof (GUID));
  MipiSystHandle.systh_tag.et_guid = 1;

  Status = CountDebugInstance (&DbgInstCount);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (RETURN_ERROR (Status)) {
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

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
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
  RETURN_STATUS     Status;
  UINT32            DbgInstCount;

  DbgInstCount = 0;

  if (NumberOfParams > sizeof (MipiSystHandle.systh_param) / sizeof (UINT32)) {
    return RETURN_INVALID_PARAMETER;
  }

  Status = MigrateTraceHubHobData ();

  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle.systh_param_count = (UINT32)NumberOfParams;
  VA_START (Args, NumberOfParams);
  for (Index = 0; Index < NumberOfParams; Index++) {
    MipiSystHandle.systh_param[Index] = VA_ARG (Args, UINT32);
  }

  VA_END (Args);

  Status = CountDebugInstance (&DbgInstCount);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               (MIPI_SYST_SEVERITY)SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 (MIPI_SYST_SEVERITY)SeverityType,
                 Id
                 );
      if (RETURN_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

/**
  Get Trace Hub MMIO Address.

  @param[in]      DgbContext        A pointer to Trace Hub debug instance.
  @param[in, out] TraceAddress      Trace Hub MMIO Address.

  @retval RETURN_SUCCESS               Get MMIO address successfully.
  @retval RETURN_INVALID_PARAMETER     TraceAddress is a NULL pointer.
**/
STATIC
RETURN_STATUS
GetTraceHubMmioAddress (
  IN     UINT8   *DgbContext,
  IN OUT UINT64  *TraceAddress
  )
{
  if (TraceAddress == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *TraceAddress = FixedPcdGet64 (PcdTraceHubDebugAddress);

  return RETURN_SUCCESS;
}

/**
  Get visibility of Trace Hub Msg.

  @param[in]      DgbContext      A pointer to Trace Hub debug instance.
  @param[in, out] Flag            Flag to enable or disable Trace Hub message.
  @param[in, out] DbgLevel        Debug Level of Trace Hub.

  @retval RETURN_SUCCESS             Get visibility of Trace Hub Msg successfully.
  @retval RETURN_INVALID_PARAMETER   On entry, Flag or DbgLevel is a NULL pointer.
**/
STATIC
RETURN_STATUS
GetTraceHubMsgVisibility (
  IN     UINT8    *DgbContext,
  IN OUT BOOLEAN  *Flag,
  IN OUT UINT8    *DbgLevel
  )
{
  if ((DbgLevel == NULL) || (Flag == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  *DbgLevel = FixedPcdGet8 (PcdTraceHubDebugLevel);
  *Flag     = FixedPcdGetBool (PcdEnableTraceHubDebugMsg);

  return RETURN_SUCCESS;
}

/**
  Collect the number of available Trace Hub debug instance.

  @param[in, out]  DbgInstCount   The number of available Trace Hub debug instance.

  @retval RETURN_SUCCESS      Collect the number of available Trace Hub debug instance successfully.
  @retval Other               Failed to collect the number of available Trace Hub debug instance.
**/
STATIC
RETURN_STATUS
CountDebugInstance (
  IN OUT UINT32  *DbgInstCount
  )
{
  if (DbgInstCount == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // 1 for fixed PCD.
  //
  *DbgInstCount = 1;

  return RETURN_SUCCESS;
}

/**
  Allocate memory to store Trace Hub HOB data.

  @retval  RETURN_SUCCESS   Migration process is successful.
  @retval  Other            Migration process is unsuccessful
**/
STATIC
RETURN_STATUS
MigrateTraceHubHobData (
  VOID
  )
{
  return RETURN_UNSUPPORTED;
}

/**
  Check whether to output Trace Hub message.

  @param[in, out]  MipiSystHandle   A pointer to MIPI_SYST_HANDLE structure.
  @param[in, out]  DgbContext       A pointer to Trace Hub debug instance.
  @param[in]       SeverityType     An error level to decide whether to enable Trace Hub data.
  @param[in]       PrintType        Either catalog print or debug print.

  @retval RETURN_SUCCESS      Current Trace Hub message need to be processed.
  @retval Other               Current Trace Hub message no need to be processed.
**/
STATIC
RETURN_STATUS
CheckWhetherToOutputMsg (
  IN OUT MIPI_SYST_HANDLE         *MipiSystHandle,
  IN OUT UINT8                    *DgbContext,
  IN     TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN     TRACEHUB_PRINTTYPE       PrintType
  )
{
  RETURN_STATUS  Status;
  UINT64         Addr;
  UINT8          DbgLevel;
  BOOLEAN        Flag;

  if (MipiSystHandle == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (PrintType == TraceHubDebugType) {
    Status = GetTraceHubMsgVisibility (NULL, &Flag, &DbgLevel);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    if (!EnableTraceHubData (Flag, DbgLevel, SeverityType)) {
      return RETURN_ABORTED;
    }
  }

  Status = GetTraceHubMmioAddress (NULL, &Addr);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr = Addr;
  if (MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr == 0) {
    return RETURN_ABORTED;
  }

  return RETURN_SUCCESS;
}
