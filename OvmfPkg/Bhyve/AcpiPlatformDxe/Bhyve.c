/*
 * Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
 * Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
 * Copyright (C) 2012, Red Hat, Inc.
 * Copyright (c) 2014, Pluribus Networks, Inc.
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#include "AcpiPlatform.h"

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BhyveFwCtlLib.h>

STATIC
EFI_STATUS
EFIAPI
BhyveInstallAcpiMadtTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  )
{
  UINT32                                              CpuCount;
  UINTN                                               cSize;
  UINTN                                               NewBufferSize;
  EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *Madt;
  EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE         *LocalApic;
  EFI_ACPI_1_0_IO_APIC_STRUCTURE                      *IoApic;
  EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE    *Iso;
  VOID                                                *Ptr;
  UINTN                                               Loop;
  EFI_STATUS                                          Status;

  ASSERT (AcpiTableBufferSize >= sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  // Query the host for the number of vCPUs
  CpuCount = 0;
  cSize = sizeof(CpuCount);
  if (BhyveFwCtlGet ("hw.ncpu", &CpuCount, &cSize) == RETURN_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Retrieved CpuCount %d\n", CpuCount));
    ASSERT (CpuCount >= 1);
  } else {
    DEBUG ((DEBUG_INFO, "CpuCount retrieval error\n"));
    CpuCount = 1;
  }

  NewBufferSize = 1                     * sizeof (*Madt) +
                  CpuCount              * sizeof (*LocalApic) +
                  1                     * sizeof (*IoApic) +
                  1                     * sizeof (*Iso);

  Madt = AllocatePool (NewBufferSize);
  if (Madt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&(Madt->Header), AcpiTableBuffer, sizeof (EFI_ACPI_DESCRIPTION_HEADER));
  Madt->Header.Length    = (UINT32) NewBufferSize;
  Madt->LocalApicAddress = 0xFEE00000;
  Madt->Flags            = EFI_ACPI_1_0_PCAT_COMPAT;
  Ptr = Madt + 1;

  LocalApic = Ptr;
  for (Loop = 0; Loop < CpuCount; ++Loop) {
    LocalApic->Type            = EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC;
    LocalApic->Length          = sizeof (*LocalApic);
    LocalApic->AcpiProcessorId = (UINT8) Loop;
    LocalApic->ApicId          = (UINT8) Loop;
    LocalApic->Flags           = 1; // enabled
    ++LocalApic;
  }
  Ptr = LocalApic;

  IoApic = Ptr;
  IoApic->Type             = EFI_ACPI_1_0_IO_APIC;
  IoApic->Length           = sizeof (*IoApic);
  IoApic->IoApicId         = (UINT8) CpuCount;
  IoApic->Reserved         = EFI_ACPI_RESERVED_BYTE;
  IoApic->IoApicAddress    = 0xFEC00000;
  IoApic->SystemVectorBase = 0x00000000;
  Ptr = IoApic + 1;

  //
  // IRQ0 (8254 Timer) => IRQ2 (PIC) Interrupt Source Override Structure
  //
  Iso = Ptr;
  Iso->Type                        = EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE;
  Iso->Length                      = sizeof (*Iso);
  Iso->Bus                         = 0x00; // ISA
  Iso->Source                      = 0x00; // IRQ0
  Iso->GlobalSystemInterruptVector = 0x00000002;
  Iso->Flags                       = 0x0000; // Conforms to specs of the bus
  Ptr = Iso + 1;

  ASSERT ((UINTN) ((UINT8 *)Ptr - (UINT8 *)Madt) == NewBufferSize);
  Status = InstallAcpiTable (AcpiProtocol, Madt, NewBufferSize, TableKey);

  FreePool (Madt);

  return Status;
}

EFI_STATUS
EFIAPI
BhyveInstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  )
{
  EFI_ACPI_DESCRIPTION_HEADER        *Hdr;
  EFI_ACPI_TABLE_INSTALL_ACPI_TABLE  TableInstallFunction;

  Hdr = (EFI_ACPI_DESCRIPTION_HEADER*) AcpiTableBuffer;
  switch (Hdr->Signature) {
  case EFI_ACPI_1_0_APIC_SIGNATURE:
    TableInstallFunction = BhyveInstallAcpiMadtTable;
    break;
  default:
    TableInstallFunction = InstallAcpiTable;
  }

  return TableInstallFunction (
           AcpiProtocol,
           AcpiTableBuffer,
           AcpiTableBufferSize,
           TableKey
           );
}
