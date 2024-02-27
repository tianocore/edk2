/** @file
System prints Trace Hub message in SEC/PEI/DXE/SMM based on fixed PCDs.
Only support single Trace Hub debug instance.

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
#include <Guid/TraceHubDebugInfoHob.h>
#include "InternalTraceHubApiCommon.h"
#include "InternalTraceHubApi.h"

/**
  Write debug string to specified Trace Hub MMIO address.

  @param[in]  SeverityType     Severity type of input message.
  @param[in]  Buffer           A pointer to the data buffer.
  @param[in]  NumberOfBytes    The size of data buffer.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTDebugWrite (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT8                    *Buffer,
  IN UINTN                    NumberOfBytes
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  RETURN_STATUS     Status;
  UINT32            DbgInstCount;
  UINT32            Index;

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return RETURN_SUCCESS;
  }

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  DbgInstCount = CountThDebugInstance ();

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               SeverityType,
               TraceHubDebugType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteDebug (
                 &MipiSystHandle,
                 SeverityType,
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

  @param[in]  SeverityType     Severity type of input message.
  @param[in]  Id               Catalog ID.
  @param[in]  Guid             Driver Guid.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTWriteCataLog64StatusCode (
  IN TRACE_HUB_SEVERITY_TYPE  SeverityType,
  IN UINT64                   Id,
  IN GUID                     *Guid
  )
{
  MIPI_SYST_HANDLE  MipiSystHandle;
  MIPI_SYST_HEADER  MipiSystHeader;
  RETURN_STATUS     Status;
  UINT32            DbgInstCount;
  UINT32            Index;

  if (Guid == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  DbgInstCount = CountThDebugInstance ();

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  SwapBytesGuid (Guid, (GUID *)(VOID *)&MipiSystHandle.systh_guid);
  MipiSystHandle.systh_tag.et_guid = 1;

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 SeverityType,
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

  @param[in]  SeverityType   Severity type of input message.
  @param[in]  Id             Catalog ID.
  @param[in]  NumberOfParams Number of entries in argument list.
  @param[in]  ...            Catalog message parameters.

  @retval RETURN_SUCCESS      Data was written to Trace Hub.
  @retval Other               Failed to output Trace Hub message.
**/
RETURN_STATUS
EFIAPI
TraceHubSysTWriteCataLog64 (
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

  DbgInstCount = CountThDebugInstance ();

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
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

  for (Index = 0; Index < DbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               NULL,
               SeverityType,
               TraceHubCatalogType
               );
    if (!RETURN_ERROR (Status)) {
      Status = MipiSystWriteCatalog (
                 &MipiSystHandle,
                 SeverityType,
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
  Collect the total number of Trace Hub debug instance in the system.

  @retval UINT32      The total number of Trace Hub debug instance in the system.
**/
UINT32
EFIAPI
CountThDebugInstance (
  VOID
  )
{
  UINT32  DbgInstCount;

  //
  // 1 from PCD.
  //
  DbgInstCount = 1;

  return DbgInstCount;
}
