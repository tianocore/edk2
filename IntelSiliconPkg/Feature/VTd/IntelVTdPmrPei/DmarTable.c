/** @file

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <IndustryStandard/Vtd.h>
#include <Ppi/VtdInfo.h>

#include "IntelVTdPmrPei.h"

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
  Get VTd engine number.

  @param[in]  AcpiDmarTable  DMAR ACPI table

  @return the VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  IN EFI_ACPI_DMAR_HEADER                    *AcpiDmarTable
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(AcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)AcpiDmarTable + AcpiDmarTable->Header.Length) {
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
  Process DMAR DHRD table.

  @param[in]  VTdInfo   The VTd engine context information.
  @param[in]  VtdIndex  The index of VTd engine.
  @param[in]  DmarDrhd  The DRHD table.
**/
VOID
ProcessDhrd (
  IN VTD_INFO                   *VTdInfo,
  IN UINTN                      VtdIndex,
  IN EFI_ACPI_DMAR_DRHD_HEADER  *DmarDrhd
  )
{
  DEBUG ((DEBUG_INFO,"  VTD (%d) BaseAddress -  0x%016lx\n", VtdIndex, DmarDrhd->RegisterBaseAddress));
  VTdInfo->VTdEngineAddress[VtdIndex] = DmarDrhd->RegisterBaseAddress;
}

/**
  Parse DMAR DRHD table.

  @param[in]  AcpiDmarTable  DMAR ACPI table

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  IN EFI_ACPI_DMAR_HEADER                    *AcpiDmarTable
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdUnitNumber;
  UINTN                                             VtdIndex;
  VTD_INFO                                          *VTdInfo;

  VtdUnitNumber = GetVtdEngineNumber (AcpiDmarTable);
  if (VtdUnitNumber == 0) {
    return EFI_UNSUPPORTED;
  }

  VTdInfo = BuildGuidHob (&mVTdInfoGuid, sizeof(VTD_INFO) + (VtdUnitNumber - 1) * sizeof(UINT64));
  ASSERT(VTdInfo != NULL);
  if (VTdInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the engine mask to all.
  //
  VTdInfo->AcpiDmarTable    = AcpiDmarTable;
  VTdInfo->EngineMask       = LShiftU64 (1, VtdUnitNumber) - 1;
  VTdInfo->HostAddressWidth = AcpiDmarTable->HostAddressWidth;
  VTdInfo->VTdEngineCount   = VtdUnitNumber;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(AcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)AcpiDmarTable + AcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      ASSERT (VtdIndex < VtdUnitNumber);
      ProcessDhrd (VTdInfo, VtdIndex, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      VtdIndex++;

      break;

    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  ASSERT (VtdIndex == VtdUnitNumber);

  return EFI_SUCCESS;
}

/**
  Return the VTd engine index according to the Segment and DevScopeEntry.

  @param AcpiDmarTable   DMAR ACPI table
  @param Segment         The segment of the VTd engine
  @param DevScopeEntry   The DevScopeEntry of the VTd engine

  @return The VTd engine index according to the Segment and DevScopeEntry.
  @retval -1  The VTd engine is not found.
**/
UINTN
GetVTdEngineFromDevScopeEntry (
  IN  EFI_ACPI_DMAR_HEADER                        *AcpiDmarTable,
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DevScopeEntry
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;
  EFI_ACPI_DMAR_DRHD_HEADER                         *DmarDrhd;
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *ThisDevScopeEntry;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(AcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)AcpiDmarTable + AcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      DmarDrhd = (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader;
      if (DmarDrhd->SegmentNumber != Segment) {
        // Mismatch
        break;
      }
      if ((DmarDrhd->Header.Length == sizeof(EFI_ACPI_DMAR_DRHD_HEADER)) ||
          ((DmarDrhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL) != 0)) {
        // No DevScopeEntry
        // Do not handle PCI_ALL
        break;
      }
      ThisDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarDrhd + 1));
      while ((UINTN)ThisDevScopeEntry < (UINTN)DmarDrhd + DmarDrhd->Header.Length) {
        if ((ThisDevScopeEntry->Length == DevScopeEntry->Length) &&
            (CompareMem (ThisDevScopeEntry, DevScopeEntry, DevScopeEntry->Length) == 0)) {
          return VtdIndex;
        }
        ThisDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)ThisDevScopeEntry + ThisDevScopeEntry->Length);
      }
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return (UINTN)-1;
}

/**
  Process DMAR RMRR table.

  @param[in]  VTdInfo   The VTd engine context information.
  @param[in]  DmarRmrr  The RMRR table.
**/
VOID
ProcessRmrr (
  IN VTD_INFO                   *VTdInfo,
  IN EFI_ACPI_DMAR_RMRR_HEADER  *DmarRmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINTN                                             VTdIndex;
  UINT64                                            RmrrMask;
  UINTN                                             LowBottom;
  UINTN                                             LowTop;
  UINTN                                             HighBottom;
  UINT64                                            HighTop;
  EFI_ACPI_DMAR_HEADER                              *AcpiDmarTable;

  AcpiDmarTable = VTdInfo->AcpiDmarTable;

  DEBUG ((DEBUG_INFO,"  RMRR (Base 0x%016lx, Limit 0x%016lx)\n", DmarRmrr->ReservedMemoryRegionBaseAddress, DmarRmrr->ReservedMemoryRegionLimitAddress));

  if ((DmarRmrr->ReservedMemoryRegionBaseAddress == 0) ||
      (DmarRmrr->ReservedMemoryRegionLimitAddress == 0)) {
    return ;
  }

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarRmrr + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarRmrr + DmarRmrr->Header.Length) {
    ASSERT (DmarDevScopeEntry->Type == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT);

    VTdIndex = GetVTdEngineFromDevScopeEntry (AcpiDmarTable, DmarRmrr->SegmentNumber, DmarDevScopeEntry);
    if (VTdIndex != (UINTN)-1) {
      RmrrMask = LShiftU64 (1, VTdIndex);

      LowBottom = 0;
      LowTop = (UINTN)DmarRmrr->ReservedMemoryRegionBaseAddress;
      HighBottom = (UINTN)DmarRmrr->ReservedMemoryRegionLimitAddress + 1;
      HighTop = LShiftU64 (1, VTdInfo->HostAddressWidth + 1);

      SetDmaProtectedRange (
        VTdInfo,
        RmrrMask,
        0,
        (UINT32)(LowTop - LowBottom),
        HighBottom,
        HighTop - HighBottom
        );

      //
      // Remove the engine from the engine mask.
      // The assumption is that any other PEI driver does not access
      // the device covered by this engine.
      //
      VTdInfo->EngineMask = VTdInfo->EngineMask & (~RmrrMask);
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }
}

/**
  Parse DMAR DRHD table.

  @param[in]  VTdInfo   The VTd engine context information.
**/
VOID
ParseDmarAcpiTableRmrr (
  IN VTD_INFO                    *VTdInfo
  )
{
  EFI_ACPI_DMAR_HEADER                    *AcpiDmarTable;
  EFI_ACPI_DMAR_STRUCTURE_HEADER          *DmarHeader;

  AcpiDmarTable = VTdInfo->AcpiDmarTable;

  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(AcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)AcpiDmarTable + AcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_RMRR:
      ProcessRmrr (VTdInfo, (EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
}
