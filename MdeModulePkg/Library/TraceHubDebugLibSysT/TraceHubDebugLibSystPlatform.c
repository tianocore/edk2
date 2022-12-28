/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MipiWrapper.h"

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x10.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD32Ts (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x10)) = Data;
}

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x18.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD32Mts (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x18)) = Data;
}

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x18.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD64Mts (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT64            Data
  )
{
  *((UINT64 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x18)) = Data;
}

/**
  Write 1 byte to Trace Hub MMIO addr + 0x0.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD8 (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT8             Data
  )
{
  *((UINT8 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x0)) = Data;
}

/**
  Write 2 bytes to Trace Hub MMIO mmio addr + 0x0.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD16 (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT16            Data
  )
{
  *((UINT16 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x0)) = Data;
}

/**
  Write 4 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD32 (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT32            Data
  )
{
  *((UINT32 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x0)) = Data;
}

/**
  Write 8 bytes to Trace Hub MMIO addr + 0x0.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Data            Data to be written.
**/
VOID
EFIAPI
SthWriteD64 (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle,
  IN  UINT64            Data
  )
{
  *((UINT64 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x0)) = Data;
}

/**
  Clear data in Trace Hub MMIO addr + 0x30.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
**/
VOID
EFIAPI
SthWriteFlag (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle
  )
{
  UINT32  *Flag;

  Flag  = (UINT32 *)(MipiSystHandle->systh_platform.TraceHubPlatformData.MmioAddr + 0x30);
  *Flag = 0;
}

/**
  Get Epoch time.

  @retval UINT64    A numeric number for timestamp.
**/
UINT64
EFIAPI
MipiSystGetEpochUs (
  VOID
  )
{
  UINT64  Epoch;

  Epoch = 1000;

  return Epoch;
}
