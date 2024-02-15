/** @file
System prints Trace Hub message in DXE/SMM based on fixed PCDs and HOB.
Trace Hub PCDs will be applied if no HOB exist.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TraceHubDebugSysTLib.h>
#include <Library/MipiSysTLib.h>
#include <Library/MipiSysTLib/mipi_syst.h>
#include <Guid/TraceHubDebugInfoHob.h>
#include "InternalTraceHubApiCommon.h"
#include "InternalTraceHubApi.h"

GLOBAL_REMOVE_IF_UNREFERENCED TRACEHUB_DEBUG_INFO_HOB  *mThDebugInstArray = NULL;
GLOBAL_REMOVE_IF_UNREFERENCED UINT32                   mDbgInstCount      = 0;

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
  UINT32            Index;

  if ((mDbgInstCount == 0) || (mThDebugInstArray == NULL)) {
    return RETURN_ABORTED;
  }

  if (NumberOfBytes == 0) {
    //
    // No data need to be written to Trace Hub
    //
    return RETURN_SUCCESS;
  }

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               (UINT8 *)&mThDebugInstArray[Index],
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
  UINTN             Index;
  RETURN_STATUS     Status;

  if ((mDbgInstCount == 0) || (mThDebugInstArray == NULL)) {
    return RETURN_ABORTED;
  }

  ZeroMem (&MipiSystHandle, sizeof (MIPI_SYST_HANDLE));
  MipiSystHandle.systh_header = &MipiSystHeader;

  Status = InitMipiSystHandle (&MipiSystHandle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (Guid != NULL) {
    SwapBytesGuid (Guid, (GUID *)(VOID *)&MipiSystHandle.systh_guid);
    MipiSystHandle.systh_tag.et_guid = 1;
  } else {
    MipiSystHandle.systh_tag.et_modunit = 2;
    MipiSystHandle.systh_tag.et_guid    = 0;
  }

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               (UINT8 *)&mThDebugInstArray[Index],
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

  if (NumberOfParams > sizeof (MipiSystHandle.systh_param) / sizeof (UINT32)) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((mDbgInstCount == 0) || (mThDebugInstArray == NULL)) {
    return RETURN_ABORTED;
  }

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

  for (Index = 0; Index < mDbgInstCount; Index++) {
    Status = CheckWhetherToOutputMsg (
               &MipiSystHandle,
               (UINT8 *)&mThDebugInstArray[Index],
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
  Constructor to get TraceHob configuration data

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval RETURN_SUCCESS           The constructor always returns EFI_SUCCESS.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough resources available to retrieve the matching FFS section.

**/
RETURN_STATUS
EFIAPI
DxeSmmTraceHubDebugSysTLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mDbgInstCount == 0) {
    mDbgInstCount = CountThDebugInstance ();
  }

  mThDebugInstArray = AllocateZeroPool (mDbgInstCount * sizeof (TRACEHUB_DEBUG_INFO_HOB));

  if (mThDebugInstArray != NULL) {
    PackThDebugInstance (mThDebugInstArray, mDbgInstCount);
  } else {
    return RETURN_OUT_OF_RESOURCES;
  }

  return RETURN_SUCCESS;
}
