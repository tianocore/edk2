/** @file
  OVMF ACPI QEMU support

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

#include "AcpiPlatform.h"
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>


BOOLEAN
QemuDetected (
  VOID
  )
{
  if (!QemuFwCfgIsAvailable ()) {
    return FALSE;
  }

  return TRUE;
}


STATIC
EFI_STATUS
EFIAPI
QemuInstallAcpiMadtTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  )
{
  EFI_STATUS                                   Status;
  UINTN                                        Count;
  UINTN                                        Loop;
  EFI_ACPI_DESCRIPTION_HEADER                  *Hdr;
  UINTN                                        NewBufferSize;
  EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE  *LocalApic;

  QemuFwCfgSelectItem (QemuFwCfgItemSmpCpuCount);
  Count = (UINTN) QemuFwCfgRead16 ();
  ASSERT (Count >= 1);

  if (Count == 1) {
    //
    // The pre-built MADT table covers the single CPU case
    //
    return InstallAcpiTable (
             AcpiProtocol,
             AcpiTableBuffer,
             AcpiTableBufferSize,
             TableKey
             );
  }

  //
  // We need to add additional Local APIC entries to the MADT
  //
  NewBufferSize = AcpiTableBufferSize + ((Count - 1) * sizeof (*LocalApic));
  Hdr = (EFI_ACPI_DESCRIPTION_HEADER*) AllocatePool (NewBufferSize);
  ASSERT (Hdr != NULL);

  CopyMem (Hdr, AcpiTableBuffer, AcpiTableBufferSize);

  LocalApic = (EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE*)
                (((UINT8*) Hdr) + AcpiTableBufferSize);

  //
  // Add Local APIC entries for the APs to the MADT
  //
  for (Loop = 1; Loop < Count; Loop++) {
    LocalApic->Type = EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC;
    LocalApic->Length = sizeof (*LocalApic);
    LocalApic->AcpiProcessorId = (UINT8) Loop;
    LocalApic->ApicId = (UINT8) Loop;
    LocalApic->Flags = 1;
    LocalApic++;
  }

  Hdr->Length = (UINT32) NewBufferSize;

  Status = InstallAcpiTable (AcpiProtocol, Hdr, NewBufferSize, TableKey);

  FreePool (Hdr);

  return Status;
}


EFI_STATUS
EFIAPI
QemuInstallAcpiTable (
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
    TableInstallFunction = QemuInstallAcpiMadtTable;
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

