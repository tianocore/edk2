/** @file


  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiPayloadEntry.h"

/**
  Find the board related info from ACPI table

  @param  AcpiTableBase          ACPI table start address in memory
  @param  AcpiBoardInfo          Pointer to the acpi board info structure

  @retval RETURN_SUCCESS     Successfully find out all the required information.
  @retval RETURN_NOT_FOUND   Failed to find the required info.

**/
RETURN_STATUS
ParseAcpiInfo (
  IN   UINT64           AcpiTableBase,
  OUT  ACPI_BOARD_INFO  *AcpiBoardInfo
  )
{
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE                                              *Fadt;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                         *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *MmCfgBase;

  Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)AcpiFindTableFromRsdp (
                                                        AcpiTableBase,
                                                        EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE
                                                        );
  MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)AcpiFindTableFromRsdp (
                                                                                 AcpiTableBase,
                                                                                 EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE
                                                                                 );

  if (Fadt == NULL) {
    return RETURN_NOT_FOUND;
  }

  AcpiBoardInfo->PmCtrlRegBase   = Fadt->Pm1aCntBlk;
  AcpiBoardInfo->PmTimerRegBase  = Fadt->PmTmrBlk;
  AcpiBoardInfo->ResetRegAddress = Fadt->ResetReg.Address;
  AcpiBoardInfo->ResetValue      = Fadt->ResetValue;
  AcpiBoardInfo->PmEvtBase       = Fadt->Pm1aEvtBlk;

  //
  // The GPE0 enable register is the upper half of a GPE0 register pair
  // that is Gpe0BlkLen bytes long.  A FADT reporting a zero length has no
  // enable register at all, and adding half of zero would name the GPE0
  // status block instead: report 0 so that consumers skip the access
  // rather than clearing status bits.
  //
  if (Fadt->Gpe0BlkLen != 0) {
    AcpiBoardInfo->PmGpeEnBase = Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2;
  } else {
    AcpiBoardInfo->PmGpeEnBase = 0;
  }

  if (MmCfgHdr != NULL) {
    MmCfgBase                      = (EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE *)((UINT8 *)MmCfgHdr + sizeof (*MmCfgHdr));
    AcpiBoardInfo->PcieBaseAddress = MmCfgBase->BaseAddress;
    AcpiBoardInfo->PcieBaseSize    = (MmCfgBase->EndBusNumber + 1 - MmCfgBase->StartBusNumber) * 4096 * 32 * 8;
  } else {
    AcpiBoardInfo->PcieBaseAddress = 0;
    AcpiBoardInfo->PcieBaseSize    = 0;
  }

  DEBUG ((DEBUG_INFO, "PmCtrl  Reg 0x%lx\n", AcpiBoardInfo->PmCtrlRegBase));
  DEBUG ((DEBUG_INFO, "PmTimer Reg 0x%lx\n", AcpiBoardInfo->PmTimerRegBase));
  DEBUG ((DEBUG_INFO, "Reset   Reg 0x%lx\n", AcpiBoardInfo->ResetRegAddress));
  DEBUG ((DEBUG_INFO, "Reset   Value 0x%x\n", AcpiBoardInfo->ResetValue));
  DEBUG ((DEBUG_INFO, "PmEvt   Reg 0x%lx\n", AcpiBoardInfo->PmEvtBase));
  DEBUG ((DEBUG_INFO, "PmGpeEn Reg 0x%lx\n", AcpiBoardInfo->PmGpeEnBase));
  DEBUG ((DEBUG_INFO, "PcieBaseAddr 0x%lx\n", AcpiBoardInfo->PcieBaseAddress));
  DEBUG ((DEBUG_INFO, "PcieBaseSize 0x%lx\n", AcpiBoardInfo->PcieBaseSize));

  return RETURN_SUCCESS;
}

/**
  Build ACPI board info HOB using infomation from ACPI table

  @param  AcpiTableBase      ACPI table start address in memory

  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  )
{
  EFI_STATUS       Status;
  ACPI_BOARD_INFO  AcpiBoardInfo;
  ACPI_BOARD_INFO  *NewAcpiBoardInfo;

  NewAcpiBoardInfo = NULL;
  Status           = ParseAcpiInfo (AcpiTableBase, &AcpiBoardInfo);
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    NewAcpiBoardInfo = BuildGuidHob (&gUefiAcpiBoardInfoGuid, sizeof (ACPI_BOARD_INFO));
    ASSERT (NewAcpiBoardInfo != NULL);
    CopyMem (NewAcpiBoardInfo, &AcpiBoardInfo, sizeof (ACPI_BOARD_INFO));
    DEBUG ((DEBUG_INFO, "Create acpi board info guid hob\n"));
  }

  return NewAcpiBoardInfo;
}
