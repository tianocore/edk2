/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MIPI_WRAPPER_H_
#define MIPI_WRAPPER_H_

#include <Uefi.h>
#include "mipi_syst.h"

typedef struct mipi_syst_handle           MIPI_SYST_HANDLE;
typedef struct mipi_syst_header           MIPI_SYST_HEADER;
typedef struct mipi_syst_msg_tag          MIPI_SYST_MSG_TAG;
typedef struct mipi_syst_guid             MIPI_SYST_GUID;
typedef struct mipi_syst_handle_flags     MIPI_SYST_HANDLE_FLAGS;
typedef struct mipi_syst_platform_handle  MIPI_SYST_PLATFORM_HANDLE;
typedef enum mipi_syst_severity           MIPI_SYST_SEVERITY;

/**
  Invoke initialization function in Mipi Sys-T module to initialize Mipi Sys-T handle.

  @param[in, out]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
**/
VOID
EFIAPI
InitMipiSystHandle (
  IN OUT MIPI_SYST_HANDLE  *MipiSystHandle
  );

/**
  Invoke write_debug_string function in Mipi Sys-T module.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  Len             Length of data buffer.
  @param[in]  Str             A pointer to data buffer.
**/
VOID
EFIAPI
WriteDebug (
  IN        MIPI_SYST_HANDLE    *MipiSystHandle,
  IN        MIPI_SYST_SEVERITY  Severity,
  IN        UINT16              Len,
  IN CONST  CHAR8               *Str
  );

/**
  Invoke catalog_write_message function in Mipi Sys-T module.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
  @param[in]  Severity        An error level to decide whether to enable Trace Hub data.
  @param[in]  CatId           Catalog Id.
**/
VOID
EFIAPI
WriteCatalog (
  IN  MIPI_SYST_HANDLE    *MipiSystHandle,
  IN  MIPI_SYST_SEVERITY  Severity,
  IN  UINT64              CatId
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Clear data in Trace Hub MMIO addr + 0x30.

  @param[in]  MipiSystHandle  A pointer to MIPI_SYST_HANDLE structure.
**/
VOID
EFIAPI
SthWriteFlag (
  IN  MIPI_SYST_HANDLE  *MipiSystHandle
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

#endif // MIPI_WRAPPER_H_
