/** @file
  The definition for DMA access Library.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DMA_ACCESS_LIB_H__
#define __DMA_ACCESS_LIB_H__

typedef struct {
  EFI_ACPI_DMAR_HEADER                    *AcpiDmarTable;
  UINT64                                  EngineMask;
  UINT8                                   HostAddressWidth;
  UINTN                                   VTdEngineCount;
  UINT64                                  VTdEngineAddress[1];
} VTD_INFO;

/**
  Set DMA protected region.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.
  @param LowMemoryBase      The protected low memory region base.
  @param LowMemoryLength    The protected low memory region length.
  @param HighMemoryBase     The protected high memory region base.
  @param HighMemoryLength   The protected high memory region length.

  @retval EFI_SUCCESS      The DMA protection is set.
  @retval EFI_UNSUPPORTED  The DMA protection is not set.
**/
EFI_STATUS
SetDmaProtectedRange (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask,
  IN UINT32        LowMemoryBase,
  IN UINT32        LowMemoryLength,
  IN UINT64        HighMemoryBase,
  IN UINT64        HighMemoryLength
  );

/**
  Diable DMA protection.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.

  @retval DMA protection is disabled.
**/
EFI_STATUS
DisableDmaProtection (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Return if the DMA protection is enabled.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.

  @retval TRUE  DMA protection is enabled in at least one VTd engine.
  @retval FALSE DMA protection is disabled in all VTd engines.
**/
UINT64
GetDmaProtectionEnabledEngineMask (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Get protected low memory alignment.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.

  @return protected low memory alignment.
**/
UINT32
GetLowMemoryAlignment (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Get protected high memory alignment.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.

  @return protected high memory alignment.
**/
UINT64
GetHighMemoryAlignment (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Enable VTd translation table protection.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.
**/
VOID
EnableVTdTranslationProtection (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Disable VTd translation table protection.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.
**/
VOID
DisableVTdTranslationProtection (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  );

/**
  Parse DMAR DRHD table.

  @param[in]  AcpiDmarTable  DMAR ACPI table

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  IN EFI_ACPI_DMAR_HEADER                    *AcpiDmarTable
  );

/**
  Parse DMAR DRHD table.

  @param VTdInfo            The VTd engine context information.
**/
VOID
ParseDmarAcpiTableRmrr (
  IN VTD_INFO                    *VTdInfo
  );

/**
  Dump DMAR ACPI table.

  @param[in]  Dmar  DMAR ACPI table
**/
VOID
DumpAcpiDMAR (
  IN EFI_ACPI_DMAR_HEADER  *Dmar
  );

extern EFI_GUID mVTdInfoGuid;

#endif

