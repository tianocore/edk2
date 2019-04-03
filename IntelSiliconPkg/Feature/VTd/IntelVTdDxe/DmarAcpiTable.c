/** @file

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DmaProtection.h"

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT32                       Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT64                       Entry;
} XSDT_TABLE;

#pragma pack()

EFI_ACPI_DMAR_HEADER  *mAcpiDmarTable = NULL;

/**
  Dump DMAR DeviceScopeEntry.

  @param[in]  DmarDeviceScopeEntry  DMAR DeviceScopeEntry
**/
VOID
DumpDmarDeviceScopeEntry (
  IN EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER     *DmarDeviceScopeEntry
  )
{
  UINTN   PciPathNumber;
  UINTN   PciPathIndex;
  EFI_ACPI_DMAR_PCI_PATH  *PciPath;

  if (DmarDeviceScopeEntry == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    *       DMA-Remapping Device Scope Entry Structure                      *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "    DMAR Device Scope Entry address ...................... 0x%016lx\n" :
    "    DMAR Device Scope Entry address ...................... 0x%08x\n",
    DmarDeviceScopeEntry
    ));
  DEBUG ((DEBUG_INFO,
    "      Device Scope Entry Type ............................ 0x%02x\n",
    DmarDeviceScopeEntry->Type
    ));
  switch (DmarDeviceScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
    DEBUG ((DEBUG_INFO,
      "        PCI Endpoint Device\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    DEBUG ((DEBUG_INFO,
      "        PCI Sub-hierachy\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
    DEBUG ((DEBUG_INFO,
      "        IOAPIC\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
    DEBUG ((DEBUG_INFO,
      "        MSI Capable HPET\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
    DEBUG ((DEBUG_INFO,
      "        ACPI Namespace Device\n"
      ));
    break;
  default:
    break;
  }
  DEBUG ((DEBUG_INFO,
    "      Length ............................................. 0x%02x\n",
    DmarDeviceScopeEntry->Length
    ));
  DEBUG ((DEBUG_INFO,
    "      Enumeration ID ..................................... 0x%02x\n",
    DmarDeviceScopeEntry->EnumerationId
    ));
  DEBUG ((DEBUG_INFO,
    "      Starting Bus Number ................................ 0x%02x\n",
    DmarDeviceScopeEntry->StartBusNumber
    ));

  PciPathNumber = (DmarDeviceScopeEntry->Length - sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER)) / sizeof(EFI_ACPI_DMAR_PCI_PATH);
  PciPath = (EFI_ACPI_DMAR_PCI_PATH *)(DmarDeviceScopeEntry + 1);
  for (PciPathIndex = 0; PciPathIndex < PciPathNumber; PciPathIndex++) {
    DEBUG ((DEBUG_INFO,
      "      Device ............................................. 0x%02x\n",
      PciPath[PciPathIndex].Device
      ));
    DEBUG ((DEBUG_INFO,
      "      Function ........................................... 0x%02x\n",
      PciPath[PciPathIndex].Function
      ));
  }

  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR ANDD table.

  @param[in]  Andd  DMAR ANDD table
**/
VOID
DumpDmarAndd (
  IN EFI_ACPI_DMAR_ANDD_HEADER *Andd
  )
{
  if (Andd == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       ACPI Name-space Device Declaration Structure                      *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  ANDD address ........................................... 0x%016lx\n" :
    "  ANDD address ........................................... 0x%08x\n",
    Andd
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Andd->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Andd->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    ACPI Device Number ................................... 0x%02x\n",
    Andd->AcpiDeviceNumber
    ));
  DEBUG ((DEBUG_INFO,
    "    ACPI Object Name ..................................... '%a'\n",
    (Andd + 1)
    ));

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR RHSA table.

  @param[in]  Rhsa  DMAR RHSA table
**/
VOID
DumpDmarRhsa (
  IN EFI_ACPI_DMAR_RHSA_HEADER *Rhsa
  )
{
  if (Rhsa == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       Remapping Hardware Status Affinity Structure                      *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  RHSA address ........................................... 0x%016lx\n" :
    "  RHSA address ........................................... 0x%08x\n",
    Rhsa
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Rhsa->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Rhsa->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Register Base Address ................................ 0x%016lx\n",
    Rhsa->RegisterBaseAddress
    ));
  DEBUG ((DEBUG_INFO,
    "    Proximity Domain ..................................... 0x%08x\n",
    Rhsa->ProximityDomain
    ));

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR ATSR table.

  @param[in]  Atsr  DMAR ATSR table
**/
VOID
DumpDmarAtsr (
  IN EFI_ACPI_DMAR_ATSR_HEADER *Atsr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDeviceScopeEntry;
  INTN                                    AtsrLen;

  if (Atsr == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       Root Port ATS Capability Reporting Structure                      *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  ATSR address ........................................... 0x%016lx\n" :
    "  ATSR address ........................................... 0x%08x\n",
    Atsr
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Atsr->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Atsr->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Atsr->Flags
    ));
  DEBUG ((DEBUG_INFO,
    "      ALL_PORTS .......................................... 0x%02x\n",
    Atsr->Flags & EFI_ACPI_DMAR_ATSR_FLAGS_ALL_PORTS
    ));
  DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Atsr->SegmentNumber
    ));

  AtsrLen  = Atsr->Header.Length - sizeof(EFI_ACPI_DMAR_ATSR_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Atsr + 1);
  while (AtsrLen > 0) {
    DumpDmarDeviceScopeEntry (DmarDeviceScopeEntry);
    AtsrLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR RMRR table.

  @param[in]  Rmrr  DMAR RMRR table
**/
VOID
DumpDmarRmrr (
  IN EFI_ACPI_DMAR_RMRR_HEADER *Rmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDeviceScopeEntry;
  INTN                                    RmrrLen;

  if (Rmrr == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       Reserved Memory Region Reporting Structure                        *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  RMRR address ........................................... 0x%016lx\n" :
    "  RMRR address ........................................... 0x%08x\n",
    Rmrr
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Rmrr->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Rmrr->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Rmrr->SegmentNumber
    ));
  DEBUG ((DEBUG_INFO,
    "    Reserved Memory Region Base Address .................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionBaseAddress
    ));
  DEBUG ((DEBUG_INFO,
    "    Reserved Memory Region Limit Address ................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionLimitAddress
    ));

  RmrrLen  = Rmrr->Header.Length - sizeof(EFI_ACPI_DMAR_RMRR_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Rmrr + 1);
  while (RmrrLen > 0) {
    DumpDmarDeviceScopeEntry (DmarDeviceScopeEntry);
    RmrrLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR DRHD table.

  @param[in]  Drhd  DMAR DRHD table
**/
VOID
DumpDmarDrhd (
  IN EFI_ACPI_DMAR_DRHD_HEADER *Drhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDeviceScopeEntry;
  INTN                                    DrhdLen;

  if (Drhd == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       DMA-Remapping Hardware Definition Structure                       *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  DRHD address ........................................... 0x%016lx\n" :
    "  DRHD address ........................................... 0x%08x\n",
    Drhd
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Drhd->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Drhd->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Drhd->Flags
    ));
  DEBUG ((DEBUG_INFO,
    "      INCLUDE_PCI_ALL .................................... 0x%02x\n",
    Drhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL
    ));
  DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Drhd->SegmentNumber
    ));
  DEBUG ((DEBUG_INFO,
    "    Register Base Address ................................ 0x%016lx\n",
    Drhd->RegisterBaseAddress
    ));

  DrhdLen  = Drhd->Header.Length - sizeof(EFI_ACPI_DMAR_DRHD_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Drhd + 1);
  while (DrhdLen > 0) {
    DumpDmarDeviceScopeEntry (DmarDeviceScopeEntry);
    DrhdLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR ACPI table.

  @param[in]  Dmar  DMAR ACPI table
**/
VOID
DumpAcpiDMAR (
  IN EFI_ACPI_DMAR_HEADER  *Dmar
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER *DmarHeader;
  INTN                  DmarLen;

  if (Dmar == NULL) {
    return;
  }

  //
  // Dump Dmar table
  //
  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "*         DMAR Table                                                        *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));

  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "DMAR address ............................................. 0x%016lx\n" :
    "DMAR address ............................................. 0x%08x\n",
    Dmar
    ));

  DEBUG ((DEBUG_INFO,
    "  Table Contents:\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    Host Address Width ................................... 0x%02x\n",
    Dmar->HostAddressWidth
    ));
  DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Dmar->Flags
    ));
  DEBUG ((DEBUG_INFO,
    "      INTR_REMAP ......................................... 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_INTR_REMAP
    ));
  DEBUG ((DEBUG_INFO,
    "      X2APIC_OPT_OUT_SET ................................. 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_X2APIC_OPT_OUT
    ));
  DEBUG ((DEBUG_INFO,
    "      DMA_CTRL_PLATFORM_OPT_IN_FLAG ...................... 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_DMA_CTRL_PLATFORM_OPT_IN_FLAG
    ));

  DmarLen  = Dmar->Header.Length - sizeof(EFI_ACPI_DMAR_HEADER);
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)(Dmar + 1);
  while (DmarLen > 0) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      DumpDmarDrhd ((EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_RMRR:
      DumpDmarRmrr ((EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_ATSR:
      DumpDmarAtsr ((EFI_ACPI_DMAR_ATSR_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_RHSA:
      DumpDmarRhsa ((EFI_ACPI_DMAR_RHSA_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_ANDD:
      DumpDmarAndd ((EFI_ACPI_DMAR_ANDD_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarLen -= DmarHeader->Length;
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }

  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR ACPI table.
**/
VOID
VtdDumpDmarTable (
  VOID
  )
{
  DumpAcpiDMAR ((EFI_ACPI_DMAR_HEADER *)(UINTN)mAcpiDmarTable);
}

/**
  Get PCI device information from DMAR DevScopeEntry.

  @param[in]  Segment               The segment number.
  @param[in]  DmarDevScopeEntry     DMAR DevScopeEntry
  @param[out] Bus                   The bus number.
  @param[out] Device                The device number.
  @param[out] Function              The function number.

  @retval EFI_SUCCESS  The PCI device information is returned.
**/
EFI_STATUS
GetPciBusDeviceFunction (
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DmarDevScopeEntry,
  OUT UINT8                                       *Bus,
  OUT UINT8                                       *Device,
  OUT UINT8                                       *Function
  )
{
  EFI_ACPI_DMAR_PCI_PATH                     *DmarPciPath;
  UINT8                                      MyBus;
  UINT8                                      MyDevice;
  UINT8                                      MyFunction;

  DmarPciPath = (EFI_ACPI_DMAR_PCI_PATH *)((UINTN)(DmarDevScopeEntry + 1));
  MyBus = DmarDevScopeEntry->StartBusNumber;
  MyDevice = DmarPciPath->Device;
  MyFunction = DmarPciPath->Function;

  switch (DmarDevScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    while ((UINTN)DmarPciPath + sizeof(EFI_ACPI_DMAR_PCI_PATH) < (UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length) {
      MyBus = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(Segment, MyBus, MyDevice, MyFunction, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      DmarPciPath ++;
      MyDevice = DmarPciPath->Device;
      MyFunction = DmarPciPath->Function;
    }
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
    break;
  }

  *Bus = MyBus;
  *Device = MyDevice;
  *Function = MyFunction;

  return EFI_SUCCESS;
}

/**
  Process DMAR DHRD table.

  @param[in]  VtdIndex  The index of VTd engine.
  @param[in]  DmarDrhd  The DRHD table.

  @retval EFI_SUCCESS The DRHD table is processed.
**/
EFI_STATUS
ProcessDhrd (
  IN UINTN                      VtdIndex,
  IN EFI_ACPI_DMAR_DRHD_HEADER  *DmarDrhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINT8                                             Bus;
  UINT8                                             Device;
  UINT8                                             Function;
  UINT8                                             SecondaryBusNumber;
  EFI_STATUS                                        Status;
  VTD_SOURCE_ID                                     SourceId;

  mVtdUnitInformation[VtdIndex].VtdUnitBaseAddress = (UINTN)DmarDrhd->RegisterBaseAddress;
  DEBUG ((DEBUG_INFO,"  VTD (%d) BaseAddress -  0x%016lx\n", VtdIndex, DmarDrhd->RegisterBaseAddress));

  mVtdUnitInformation[VtdIndex].Segment = DmarDrhd->SegmentNumber;

  if ((DmarDrhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL) != 0) {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag = TRUE;
    DEBUG ((DEBUG_INFO,"  ProcessDhrd: with INCLUDE ALL\n"));

    Status = ScanPciBus((VOID *)VtdIndex, DmarDrhd->SegmentNumber, 0, ScanBusCallbackRegisterPciDevice);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    mVtdUnitInformation[VtdIndex].PciDeviceInfo.IncludeAllFlag = FALSE;
    DEBUG ((DEBUG_INFO,"  ProcessDhrd: without INCLUDE ALL\n"));
  }

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarDrhd + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarDrhd + DmarDrhd->Header.Length) {

    Status = GetPciBusDeviceFunction (DmarDrhd->SegmentNumber, DmarDevScopeEntry, &Bus, &Device, &Function);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_INFO,"  ProcessDhrd: "));
    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      DEBUG ((DEBUG_INFO,"PCI Endpoint"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      DEBUG ((DEBUG_INFO,"PCI-PCI bridge"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
      DEBUG ((DEBUG_INFO,"IOAPIC"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
      DEBUG ((DEBUG_INFO,"MSI Capable HPET"));
      break;
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
      DEBUG ((DEBUG_INFO,"ACPI Namespace Device"));
      break;
    }
    DEBUG ((DEBUG_INFO," S%04x B%02x D%02x F%02x\n", DmarDrhd->SegmentNumber, Bus, Device, Function));

    SourceId.Bits.Bus = Bus;
    SourceId.Bits.Device = Device;
    SourceId.Bits.Function = Function;

    Status = RegisterPciDevice (VtdIndex, DmarDrhd->SegmentNumber, SourceId, DmarDevScopeEntry->Type, TRUE);
    if (EFI_ERROR (Status)) {
      //
      // There might be duplication for special device other than standard PCI device.
      //
      switch (DmarDevScopeEntry->Type) {
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
      case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
        return Status;
      }
    }

    switch (DmarDevScopeEntry->Type) {
    case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
      SecondaryBusNumber = PciSegmentRead8 (PCI_SEGMENT_LIB_ADDRESS(DmarDrhd->SegmentNumber, Bus, Device, Function, PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET));
      Status = ScanPciBus ((VOID *)VtdIndex, DmarDrhd->SegmentNumber, SecondaryBusNumber, ScanBusCallbackRegisterPciDevice);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    default:
      break;
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }

  return EFI_SUCCESS;
}

/**
  Process DMAR RMRR table.

  @param[in]  DmarRmrr  The RMRR table.

  @retval EFI_SUCCESS The RMRR table is processed.
**/
EFI_STATUS
ProcessRmrr (
  IN EFI_ACPI_DMAR_RMRR_HEADER  *DmarRmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINT8                                             Bus;
  UINT8                                             Device;
  UINT8                                             Function;
  EFI_STATUS                                        Status;
  VTD_SOURCE_ID                                     SourceId;

  DEBUG ((DEBUG_INFO,"  RMRR (Base 0x%016lx, Limit 0x%016lx)\n", DmarRmrr->ReservedMemoryRegionBaseAddress, DmarRmrr->ReservedMemoryRegionLimitAddress));

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarRmrr + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarRmrr + DmarRmrr->Header.Length) {
    if (DmarDevScopeEntry->Type != EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT) {
      DEBUG ((DEBUG_INFO,"RMRR DevScopeEntryType is not endpoint, type[0x%x] \n", DmarDevScopeEntry->Type));
      return EFI_DEVICE_ERROR;
    }

    Status = GetPciBusDeviceFunction (DmarRmrr->SegmentNumber, DmarDevScopeEntry, &Bus, &Device, &Function);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_INFO,"RMRR S%04x B%02x D%02x F%02x\n", DmarRmrr->SegmentNumber, Bus, Device, Function));

    SourceId.Bits.Bus = Bus;
    SourceId.Bits.Device = Device;
    SourceId.Bits.Function = Function;
    Status = SetAccessAttribute (
               DmarRmrr->SegmentNumber,
               SourceId,
               DmarRmrr->ReservedMemoryRegionBaseAddress,
               DmarRmrr->ReservedMemoryRegionLimitAddress + 1 - DmarRmrr->ReservedMemoryRegionBaseAddress,
               EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }

  return EFI_SUCCESS;
}

/**
  Get VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdIndex++;
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return VtdIndex ;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  EFI_STATUS                                        Status;
  UINTN                                             VtdIndex;

  mVtdUnitNumber = GetVtdEngineNumber ();
  DEBUG ((DEBUG_INFO,"  VtdUnitNumber - %d\n", mVtdUnitNumber));
  ASSERT (mVtdUnitNumber > 0);
  if (mVtdUnitNumber == 0) {
    return EFI_DEVICE_ERROR;
  }

  mVtdUnitInformation = AllocateZeroPool (sizeof(*mVtdUnitInformation) * mVtdUnitNumber);
  ASSERT (mVtdUnitInformation != NULL);
  if (mVtdUnitInformation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      ASSERT (VtdIndex < mVtdUnitNumber);
      Status = ProcessDhrd (VtdIndex, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      VtdIndex++;

      break;

    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  ASSERT (VtdIndex == mVtdUnitNumber);

  for (VtdIndex = 0; VtdIndex < mVtdUnitNumber; VtdIndex++) {
    DumpPciDeviceInfo (VtdIndex);
  }
  return EFI_SUCCESS ;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableRmrr (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  EFI_STATUS                                        Status;

  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_RMRR:
      Status = ProcessRmrr ((EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return EFI_SUCCESS ;
}

/**
  Get the DMAR ACPI table.

  @retval EFI_SUCCESS           The DMAR ACPI table is got.
  @retval EFI_ALREADY_STARTED   The DMAR ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The DMAR ACPI table is not found.
**/
EFI_STATUS
GetDmarAcpiTable (
  VOID
  )
{
  if (mAcpiDmarTable != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mAcpiDmarTable = (EFI_ACPI_DMAR_HEADER *) EfiLocateFirstAcpiTable (
                                              EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE
                                              );
  if (mAcpiDmarTable == NULL) {
    return EFI_NOT_FOUND;
  }
  DEBUG ((DEBUG_INFO,"DMAR Table - 0x%08x\n", mAcpiDmarTable));
  VtdDumpDmarTable();

  return EFI_SUCCESS;
}
