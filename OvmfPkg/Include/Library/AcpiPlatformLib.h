/** @file
  Copyright (c) 2023, Corvin Köhne <corvink@FreeBSD.org>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_PLATFORM_LIB_H_
#define ACPI_PLATFORM_LIB_H_

#include <Protocol/AcpiTable.h>
#include <Protocol/PciIo.h>

typedef struct {
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT64                 PciAttributes;
} ORIGINAL_ATTRIBUTES;

typedef struct S3_CONTEXT S3_CONTEXT;

/**
  Searches and returns the address of the ACPI Root System Description Pointer (RSDP) in system memory.

  @param  StartAddress        Start address of search range.
  @param  EndAddress          End address of search range.
  @param  RsdpPtr             Return pointer to RSDP.

  @retval EFI_SUCCESS         RSDP successfully found.
  @retval EFI_NOT_FOUND       Couldn't find RSDP.
  @retval EFI_ABORTED         Invalid RSDP found.
**/
EFI_STATUS
EFIAPI
GetAcpiRsdpFromMemory (
  IN UINTN                                          StartAddress,
  IN UINTN                                          EndAddress,
  OUT EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  **RsdpPtr
  );

/**
  Get Acpi tables from the RSDP structure. And installs ACPI tables
  into the RSDT/XSDT using InstallAcpiTable. Some signature of the installed
  ACPI tables are: FACP, APIC, HPET, WAET, SSDT, FACS, DSDT.

  @param  AcpiProtocol           Protocol instance pointer.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableHandle is
                                 NULL, or AcpiTableBufferSize and the size
                                 field embedded in the ACPI table pointed to
                                 by AcpiTableBuffer are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the
request.

**/
EFI_STATUS
EFIAPI
InstallAcpiTablesFromRsdp (
  IN EFI_ACPI_TABLE_PROTOCOL                       *AcpiProtocol,
  IN EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp
  );

EFI_STATUS
EFIAPI
InstallQemuFwCfgTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiProtocol
  );

VOID
EnablePciDecoding (
  OUT ORIGINAL_ATTRIBUTES  **OriginalAttributes,
  OUT UINTN                *Count
  );

VOID
RestorePciDecoding (
  IN ORIGINAL_ATTRIBUTES  *OriginalAttributes,
  IN UINTN                Count
  );

EFI_STATUS
AllocateS3Context (
  OUT S3_CONTEXT  **S3Context,
  IN  UINTN       WritePointerCount
  );

VOID
ReleaseS3Context (
  IN S3_CONTEXT  *S3Context
  );

EFI_STATUS
SaveCondensedWritePointerToS3Context (
  IN OUT S3_CONTEXT  *S3Context,
  IN     UINT16      PointerItem,
  IN     UINT8       PointerSize,
  IN     UINT32      PointerOffset,
  IN     UINT64      PointerValue
  );

EFI_STATUS
TransferS3ContextToBootScript (
  IN S3_CONTEXT  *S3Context
  );

#endif
