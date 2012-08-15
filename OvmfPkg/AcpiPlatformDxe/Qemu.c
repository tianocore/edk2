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
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/Acpi.h>

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
UINTN
CountBits16 (
  UINT16 Mask
  )
{
  //
  // For all N >= 1, N bits are enough to represent the number of bits set
  // among N bits. It's true for N == 1. When adding a new bit (N := N+1),
  // the maximum number of possibly set bits increases by one, while the
  // representable maximum doubles.
  //
  Mask = ((Mask & 0xAAAA) >> 1) + (Mask & 0x5555);
  Mask = ((Mask & 0xCCCC) >> 2) + (Mask & 0x3333);
  Mask = ((Mask & 0xF0F0) >> 4) + (Mask & 0x0F0F);
  Mask = ((Mask & 0xFF00) >> 8) + (Mask & 0x00FF);

  return Mask;
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
  UINTN                                               CpuCount;
  UINTN                                               PciLinkIsoCount;
  UINTN                                               NewBufferSize;
  EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *Madt;
  EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE         *LocalApic;
  EFI_ACPI_1_0_IO_APIC_STRUCTURE                      *IoApic;
  EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE    *Iso;
  EFI_ACPI_1_0_LOCAL_APIC_NMI_STRUCTURE               *LocalApicNmi;
  VOID                                                *Ptr;
  UINTN                                               Loop;
  EFI_STATUS                                          Status;

  ASSERT (AcpiTableBufferSize >= sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  QemuFwCfgSelectItem (QemuFwCfgItemSmpCpuCount);
  CpuCount = QemuFwCfgRead16 ();
  ASSERT (CpuCount >= 1);

  //
  // Set Level-tiggered, Active High for these identity mapped IRQs. The bitset
  // corresponds to the union of all possible interrupt assignments for the LNKA,
  // LNKB, LNKC, LNKD PCI interrupt lines. See the DSDT.
  //
  PciLinkIsoCount = CountBits16 (PcdGet16 (Pcd8259LegacyModeEdgeLevel));

  NewBufferSize = 1                     * sizeof (*Madt) +
                  CpuCount              * sizeof (*LocalApic) +
                  1                     * sizeof (*IoApic) +
                  (1 + PciLinkIsoCount) * sizeof (*Iso) +
                  1                     * sizeof (*LocalApicNmi);

  Madt = AllocatePool (NewBufferSize);
  if (Madt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&(Madt->Header), AcpiTableBuffer, sizeof (EFI_ACPI_DESCRIPTION_HEADER));
  Madt->Header.Length    = (UINT32) NewBufferSize;
  Madt->LocalApicAddress = PcdGet32 (PcdCpuLocalApicBaseAddress);
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
  ++Iso;

  //
  // Set Level-tiggered, Active High for all possible PCI link targets.
  //
  for (Loop = 0; Loop < 16; ++Loop) {
    if ((PcdGet16 (Pcd8259LegacyModeEdgeLevel) & (1 << Loop)) == 0) {
      continue;
    }
    Iso->Type                        = EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE;
    Iso->Length                      = sizeof (*Iso);
    Iso->Bus                         = 0x00; // ISA
    Iso->Source                      = (UINT8) Loop;
    Iso->GlobalSystemInterruptVector = (UINT32) Loop;
    Iso->Flags                       = 0x000D; // Level-tiggered, Active High
    ++Iso;
  }
  ASSERT (
    (UINTN) (Iso - (EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE *)Ptr) ==
      1 + PciLinkIsoCount
    );
  Ptr = Iso;

  LocalApicNmi = Ptr;
  LocalApicNmi->Type            = EFI_ACPI_1_0_LOCAL_APIC_NMI;
  LocalApicNmi->Length          = sizeof (*LocalApicNmi);
  LocalApicNmi->AcpiProcessorId = 0xFF; // applies to all processors
  //
  // polarity and trigger mode of the APIC I/O input signals conform to the
  // specifications of the bus
  //
  LocalApicNmi->Flags           = 0x0000;
  //
  // Local APIC interrupt input LINTn to which NMI is connected.
  //
  LocalApicNmi->LocalApicInti   = 0x01;
  Ptr = LocalApicNmi + 1;

  ASSERT ((UINTN) ((UINT8 *)Ptr - (UINT8 *)Madt) == NewBufferSize);
  Status = InstallAcpiTable (AcpiProtocol, Madt, NewBufferSize, TableKey);

  FreePool (Madt);

  return Status;
}


#pragma pack(1)

typedef struct {
  UINT64 Base;
  UINT64 End;
  UINT64 Length;
} PCI_WINDOW;

typedef struct {
  PCI_WINDOW PciWindow32;
  PCI_WINDOW PciWindow64;
} FIRMWARE_DATA;

#pragma pack()


STATIC
EFI_STATUS
EFIAPI
PopulateFwData(
  OUT  FIRMWARE_DATA *FwData
  )
{
  EFI_STATUS                      Status;
  UINTN                           NumDesc;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *AllDesc;

  Status = gDS->GetMemorySpaceMap (&NumDesc, &AllDesc);
  if (Status == EFI_SUCCESS) {
    UINT64 NonMmio32MaxExclTop;
    UINT64 Mmio32MinBase;
    UINT64 Mmio32MaxExclTop;
    UINTN CurDesc;

    Status = EFI_UNSUPPORTED;

    NonMmio32MaxExclTop = 0;
    Mmio32MinBase = BASE_4GB;
    Mmio32MaxExclTop = 0;

    for (CurDesc = 0; CurDesc < NumDesc; ++CurDesc) {
      CONST EFI_GCD_MEMORY_SPACE_DESCRIPTOR *Desc;
      UINT64 ExclTop;

      Desc = &AllDesc[CurDesc];
      ExclTop = Desc->BaseAddress + Desc->Length;

      if (ExclTop <= BASE_4GB) {
        switch (Desc->GcdMemoryType) {
          case EfiGcdMemoryTypeNonExistent:
            break;

          case EfiGcdMemoryTypeReserved:
          case EfiGcdMemoryTypeSystemMemory:
            if (NonMmio32MaxExclTop < ExclTop) {
              NonMmio32MaxExclTop = ExclTop;
            }
            break;

          case EfiGcdMemoryTypeMemoryMappedIo:
            if (Mmio32MinBase > Desc->BaseAddress) {
              Mmio32MinBase = Desc->BaseAddress;
            }
            if (Mmio32MaxExclTop < ExclTop) {
              Mmio32MaxExclTop = ExclTop;
            }
            break;

          default:
            ASSERT(0);
        }
      }
    }

    if (Mmio32MinBase < NonMmio32MaxExclTop) {
      Mmio32MinBase = NonMmio32MaxExclTop;
    }

    if (Mmio32MinBase < Mmio32MaxExclTop) {
      FwData->PciWindow32.Base   = Mmio32MinBase;
      FwData->PciWindow32.End    = Mmio32MaxExclTop - 1;
      FwData->PciWindow32.Length = Mmio32MaxExclTop - Mmio32MinBase;

      FwData->PciWindow64.Base   = 0;
      FwData->PciWindow64.End    = 0;
      FwData->PciWindow64.Length = 0;

      Status = EFI_SUCCESS;
    }

    FreePool (AllDesc);
  }

  DEBUG ((
    DEBUG_INFO,
    "ACPI PciWindow32: Base=0x%08lx End=0x%08lx Length=0x%08lx\n",
    FwData->PciWindow32.Base,
    FwData->PciWindow32.End,
    FwData->PciWindow32.Length
    ));
  DEBUG ((
    DEBUG_INFO,
    "ACPI PciWindow64: Base=0x%08lx End=0x%08lx Length=0x%08lx\n",
    FwData->PciWindow64.Base,
    FwData->PciWindow64.End,
    FwData->PciWindow64.Length
    ));

  return Status;
}


STATIC
EFI_STATUS
EFIAPI
QemuInstallAcpiSsdtTable (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol,
  IN   VOID                          *AcpiTableBuffer,
  IN   UINTN                         AcpiTableBufferSize,
  OUT  UINTN                         *TableKey
  )
{
  EFI_STATUS    Status;
  FIRMWARE_DATA *FwData;

  Status = EFI_OUT_OF_RESOURCES;

  FwData = AllocateReservedPool (sizeof (*FwData));
  if (FwData != NULL) {
    UINTN SsdtSize;
    UINT8 *Ssdt;

    SsdtSize = AcpiTableBufferSize + 17;
    Ssdt = AllocatePool (SsdtSize);

    if (Ssdt != NULL) {
      Status = PopulateFwData (FwData);

      if (Status == EFI_SUCCESS) {
        UINT8 *SsdtPtr;

        SsdtPtr = Ssdt;

        CopyMem (SsdtPtr, AcpiTableBuffer, AcpiTableBufferSize);
        SsdtPtr += AcpiTableBufferSize;

        //
        // build "OperationRegion(FWDT, SystemMemory, 0x12345678, 0x87654321)"
        //
        *(SsdtPtr++) = 0x5B; // ExtOpPrefix
        *(SsdtPtr++) = 0x80; // OpRegionOp
        *(SsdtPtr++) = 'F';
        *(SsdtPtr++) = 'W';
        *(SsdtPtr++) = 'D';
        *(SsdtPtr++) = 'T';
        *(SsdtPtr++) = 0x00; // SystemMemory
        *(SsdtPtr++) = 0x0C; // DWordPrefix

        //
        // no virtual addressing yet, take the four least significant bytes
        //
        CopyMem(SsdtPtr, &FwData, 4);
        SsdtPtr += 4;

        *(SsdtPtr++) = 0x0C; // DWordPrefix

        *(UINT32*) SsdtPtr = sizeof (*FwData);
        SsdtPtr += 4;

        ASSERT((UINTN) (SsdtPtr - Ssdt) == SsdtSize);
        ((EFI_ACPI_DESCRIPTION_HEADER *) Ssdt)->Length = (UINT32) SsdtSize;
        Status = InstallAcpiTable (AcpiProtocol, Ssdt, SsdtSize, TableKey);
      }

      FreePool(Ssdt);
    }

    if (Status != EFI_SUCCESS) {
      FreePool(FwData);
    }
  }

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
  case EFI_ACPI_1_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
    TableInstallFunction = QemuInstallAcpiSsdtTable;
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

