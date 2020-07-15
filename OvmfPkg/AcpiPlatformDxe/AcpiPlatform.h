/** @file
  Sample ACPI Platform Driver

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_PLATFORM_H_INCLUDED_
#define _ACPI_PLATFORM_H_INCLUDED_

#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/XenPlatformLib.h>

#include <IndustryStandard/Acpi.h>

typedef struct {
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT64              PciAttributes;
} ORIGINAL_ATTRIBUTES;

typedef struct S3_CONTEXT S3_CONTEXT;

EFI_STATUS
EFIAPI
InstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  );

BOOLEAN
QemuDetected (
  VOID
  );

EFI_STATUS
EFIAPI
QemuInstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  );

EFI_STATUS
EFIAPI
InstallXenTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  );

EFI_STATUS
EFIAPI
InstallQemuFwCfgTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  );

EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiTable
  );

VOID
EnablePciDecoding (
  OUT ORIGINAL_ATTRIBUTES **OriginalAttributes,
  OUT UINTN               *Count
  );

VOID
RestorePciDecoding (
  IN ORIGINAL_ATTRIBUTES *OriginalAttributes,
  IN UINTN               Count
  );

EFI_STATUS
AllocateS3Context (
  OUT S3_CONTEXT **S3Context,
  IN  UINTN      WritePointerCount
  );

VOID
ReleaseS3Context (
  IN S3_CONTEXT *S3Context
  );

EFI_STATUS
SaveCondensedWritePointerToS3Context (
  IN OUT S3_CONTEXT *S3Context,
  IN     UINT16     PointerItem,
  IN     UINT8      PointerSize,
  IN     UINT32     PointerOffset,
  IN     UINT64     PointerValue
  );

EFI_STATUS
TransferS3ContextToBootScript (
  IN S3_CONTEXT *S3Context
  );

#endif

