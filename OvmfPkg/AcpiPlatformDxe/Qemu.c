/** @file
  OVMF ACPI QEMU support

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>

  Copyright (C) 2012-2014, Red Hat, Inc.

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

typedef struct {
  UINT8 BytePrefix;
  UINT8 ByteValue;
} AML_BYTE;

typedef struct {
  UINT8    NameOp;
  UINT8    RootChar;
  UINT8    NameChar[4];
  UINT8    PackageOp;
  UINT8    PkgLength;
  UINT8    NumElements;
  AML_BYTE Pm1aCntSlpTyp;
  AML_BYTE Pm1bCntSlpTyp;
  AML_BYTE Reserved[2];
} SYSTEM_STATE_PACKAGE;

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

      if (ExclTop <= (UINT64) PcdGet32 (PcdOvmfFdBaseAddress)) {
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
VOID
EFIAPI
GetSuspendStates (
  UINTN                *SuspendToRamSize,
  SYSTEM_STATE_PACKAGE *SuspendToRam,
  UINTN                *SuspendToDiskSize,
  SYSTEM_STATE_PACKAGE *SuspendToDisk
  )
{
  STATIC CONST SYSTEM_STATE_PACKAGE Template = {
    0x08,                   // NameOp
    '\\',                   // RootChar
    { '_', 'S', 'x', '_' }, // NameChar[4]
    0x12,                   // PackageOp
    0x0A,                   // PkgLength
    0x04,                   // NumElements
    { 0x0A, 0x00 },         // Pm1aCntSlpTyp
    { 0x0A, 0x00 },         // Pm1bCntSlpTyp -- we don't support it
    {                       // Reserved[2]
      { 0x0A, 0x00 },
      { 0x0A, 0x00 }
    }
  };
  RETURN_STATUS                     Status;
  FIRMWARE_CONFIG_ITEM              FwCfgItem;
  UINTN                             FwCfgSize;
  UINT8                             SystemStates[6];

  //
  // configure defaults
  //
  *SuspendToRamSize = sizeof Template;
  CopyMem (SuspendToRam, &Template, sizeof Template);
  SuspendToRam->NameChar[2]             = '3'; // S3
  SuspendToRam->Pm1aCntSlpTyp.ByteValue = 1;   // PIIX4: STR

  *SuspendToDiskSize = sizeof Template;
  CopyMem (SuspendToDisk, &Template, sizeof Template);
  SuspendToDisk->NameChar[2]             = '4'; // S4
  SuspendToDisk->Pm1aCntSlpTyp.ByteValue = 2;   // PIIX4: POSCL

  //
  // check for overrides
  //
  Status = QemuFwCfgFindFile ("etc/system-states", &FwCfgItem, &FwCfgSize);
  if (Status != RETURN_SUCCESS || FwCfgSize != sizeof SystemStates) {
    DEBUG ((DEBUG_INFO, "ACPI using S3/S4 defaults\n"));
    return;
  }
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof SystemStates, SystemStates);

  //
  // Each byte corresponds to a system state. In each byte, the MSB tells us
  // whether the given state is enabled. If so, the three LSBs specify the
  // value to be written to the PM control register's SUS_TYP bits.
  //
  if (SystemStates[3] & BIT7) {
    SuspendToRam->Pm1aCntSlpTyp.ByteValue =
        SystemStates[3] & (BIT2 | BIT1 | BIT0);
    DEBUG ((DEBUG_INFO, "ACPI S3 value: %d\n",
            SuspendToRam->Pm1aCntSlpTyp.ByteValue));
  } else {
    *SuspendToRamSize = 0;
    DEBUG ((DEBUG_INFO, "ACPI S3 disabled\n"));
  }

  if (SystemStates[4] & BIT7) {
    SuspendToDisk->Pm1aCntSlpTyp.ByteValue =
        SystemStates[4] & (BIT2 | BIT1 | BIT0);
    DEBUG ((DEBUG_INFO, "ACPI S4 value: %d\n",
            SuspendToDisk->Pm1aCntSlpTyp.ByteValue));
  } else {
    *SuspendToDiskSize = 0;
    DEBUG ((DEBUG_INFO, "ACPI S4 disabled\n"));
  }
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
    UINTN                SuspendToRamSize;
    SYSTEM_STATE_PACKAGE SuspendToRam;
    UINTN                SuspendToDiskSize;
    SYSTEM_STATE_PACKAGE SuspendToDisk;
    UINTN                SsdtSize;
    UINT8                *Ssdt;

    GetSuspendStates (&SuspendToRamSize,  &SuspendToRam,
                      &SuspendToDiskSize, &SuspendToDisk);
    SsdtSize = AcpiTableBufferSize + 17 + SuspendToRamSize + SuspendToDiskSize;
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

        //
        // add suspend system states
        //
        CopyMem (SsdtPtr, &SuspendToRam, SuspendToRamSize);
        SsdtPtr += SuspendToRamSize;
        CopyMem (SsdtPtr, &SuspendToDisk, SuspendToDiskSize);
        SsdtPtr += SuspendToDiskSize;

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


//
// We'll be saving the keys of installed tables so that we can roll them back
// in case of failure. 128 tables should be enough for anyone (TM).
//
#define INSTALLED_TABLES_MAX 128

/**
  Download one ACPI table data file from QEMU and interpret it.

  @param[in] FwCfgFile         The NUL-terminated name of the fw_cfg file to
                               download and interpret.

  @param[in] AcpiProtocol      The ACPI table protocol used to install tables.

  @param[in,out] InstalledKey  On input, an array of INSTALLED_TABLES_MAX UINTN
                               elements, allocated by the caller. On output,
                               the function will have stored (appended) the
                               AcpiProtocol-internal keys of the ACPI tables
                               that the function has installed from the fw_cfg
                               file. The array reflects installed tables even
                               if the function returns with an error.

  @param[in,out] NumInstalled  On input, the number of entries already used in
                               InstalledKey; it must be in [0,
                               INSTALLED_TABLES_MAX] inclusive. On output, the
                               parameter is updated to the new cumulative count
                               of the keys stored in InstalledKey; the value
                               reflects installed tables even if the function
                               returns with an error.

  @retval  EFI_INVALID_PARAMETER  NumInstalled is outside the allowed range on
                                  input.

  @retval  EFI_UNSUPPORTED        Firmware configuration is unavailable.

  @retval  EFI_NOT_FOUND          The host doesn't export the requested fw_cfg
                                  file.

  @retval  EFI_OUT_OF_RESOURCES   Memory allocation failed, or no more room in
                                  InstalledKey.

  @retval  EFI_PROTOCOL_ERROR     Found truncated or invalid ACPI table header
                                  in the fw_cfg contents.

  @return                         Status codes returned by
                                  AcpiProtocol->InstallAcpiTable().

**/

STATIC
EFI_STATUS
InstallQemuLinkedTables (
  IN     CONST CHAR8             *FwCfgFile,
  IN     EFI_ACPI_TABLE_PROTOCOL *AcpiProtocol,
  IN OUT UINTN                   InstalledKey[INSTALLED_TABLES_MAX],
  IN OUT INT32                   *NumInstalled
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM TablesFile;
  UINTN                TablesFileSize;
  UINT8                *Tables;
  UINTN                Processed;

  if (*NumInstalled < 0 || *NumInstalled > INSTALLED_TABLES_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  Status = QemuFwCfgFindFile (FwCfgFile, &TablesFile, &TablesFileSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: \"%a\" unavailable: %r\n", __FUNCTION__,
      FwCfgFile, Status));
    return Status;
  }

  Tables = AllocatePool (TablesFileSize);
  if (Tables == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (TablesFile);
  QemuFwCfgReadBytes (TablesFileSize, Tables);

  Processed = 0;
  while (Processed < TablesFileSize) {
    UINTN                       Remaining;
    EFI_ACPI_DESCRIPTION_HEADER *Probe;

    Remaining = TablesFileSize - Processed;
    if (Remaining < sizeof *Probe) {
      Status = EFI_PROTOCOL_ERROR;
      break;
    }

    Probe = (EFI_ACPI_DESCRIPTION_HEADER *) (Tables + Processed);
    if (Remaining < Probe->Length || Probe->Length < sizeof *Probe) {
      Status = EFI_PROTOCOL_ERROR;
      break;
    }

    DEBUG ((EFI_D_VERBOSE, "%a: \"%a\" offset 0x%016Lx:"
      " Signature=\"%-4.4a\" Length=0x%08x\n",
      __FUNCTION__, FwCfgFile, (UINT64) Processed,
      (CONST CHAR8 *) &Probe->Signature, Probe->Length));

    //
    // skip automatically handled "root" tables: RSDT, XSDT
    //
    if (Probe->Signature !=
                        EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE &&
        Probe->Signature !=
                    EFI_ACPI_2_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
      if (*NumInstalled == INSTALLED_TABLES_MAX) {
        DEBUG ((EFI_D_ERROR, "%a: can't install more than %d tables\n",
          __FUNCTION__, INSTALLED_TABLES_MAX));
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      Status = AcpiProtocol->InstallAcpiTable (AcpiProtocol, Probe,
                 Probe->Length, &InstalledKey[*NumInstalled]);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR,
          "%a: failed to install table \"%-4.4a\" at \"%a\" offset 0x%Lx: "
          "%r\n", __FUNCTION__, (CONST CHAR8 *)&Probe->Signature, FwCfgFile,
          (UINT64) Processed, Status));
        break;
      }

      ++*NumInstalled;
    }

    Processed += Probe->Length;
  }

  //
  // NUL-padding at the end is accepted
  //
  if (Status == EFI_PROTOCOL_ERROR) {
    UINTN ErrorLocation;

    ErrorLocation = Processed;
    while (Processed < TablesFileSize && Tables[Processed] == '\0') {
      ++Processed;
    }
    if (Processed < TablesFileSize) {
      DEBUG ((EFI_D_ERROR, "%a: truncated or invalid ACPI table header at "
        "\"%a\" offset 0x%Lx\n", __FUNCTION__, FwCfgFile,
        (UINT64)ErrorLocation));
    }
  }

  if (Processed == TablesFileSize) {
    Status = EFI_SUCCESS;
  } else {
    ASSERT (EFI_ERROR (Status));
  }

  FreePool (Tables);
  return Status;
}

/**
  Download all ACPI table data files from QEMU and interpret them.

  @param[in] AcpiProtocol  The ACPI table protocol used to install tables.

  @retval  EFI_UNSUPPORTED       Firmware configuration is unavailable.

  @retval  EFI_NOT_FOUND         The host doesn't export the required fw_cfg
                                 files.

  @retval  EFI_OUT_OF_RESOURCES  Memory allocation failed, or more than
                                 INSTALLED_TABLES_MAX tables found.

  @retval  EFI_PROTOCOL_ERROR    Found truncated or invalid ACPI table header
                                 in the fw_cfg contents.

  @return                        Status codes returned by
                                 AcpiProtocol->InstallAcpiTable().

**/

EFI_STATUS
EFIAPI
InstallAllQemuLinkedTables (
  IN   EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
  )
{
  UINTN                *InstalledKey;
  INT32                Installed;
  EFI_STATUS           Status;

  InstalledKey = AllocatePool (INSTALLED_TABLES_MAX * sizeof *InstalledKey);
  if (InstalledKey == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Installed = 0;

  Status = InstallQemuLinkedTables ("etc/acpi/tables", AcpiProtocol,
             InstalledKey, &Installed);
  if (EFI_ERROR (Status)) {
    ASSERT (Status != EFI_INVALID_PARAMETER);
    //
    // Roll back partial installation.
    //
    while (Installed > 0) {
      --Installed;
      AcpiProtocol->UninstallAcpiTable (AcpiProtocol, InstalledKey[Installed]);
    }
  } else {
    DEBUG ((EFI_D_INFO, "%a: installed %d tables\n", __FUNCTION__, Installed));
  }

  FreePool (InstalledKey);
  return Status;
}
