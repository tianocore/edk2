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
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER                                           *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                                                            *Rsdt;
  UINT32                                                                                 *Entry32;
  UINTN                                                                                  Entry32Num;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE                                              *Fadt;
  EFI_ACPI_DESCRIPTION_HEADER                                                            *Xsdt;
  UINT64                                                                                 *Entry64;
  UINTN                                                                                  Entry64Num;
  UINTN                                                                                  Idx;
  UINT32                                                                                 *Signature;
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER                         *MmCfgHdr;
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE  *MmCfgBase;

  Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)AcpiTableBase;
  DEBUG ((DEBUG_INFO, "Rsdp at 0x%p\n", Rsdp));
  DEBUG ((DEBUG_INFO, "Rsdt at 0x%x, Xsdt at 0x%lx\n", Rsdp->RsdtAddress, Rsdp->XsdtAddress));

  //
  // Search Rsdt First
  //
  Fadt     = NULL;
  MmCfgHdr = NULL;
  Rsdt     = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->RsdtAddress);
  if (Rsdt != NULL) {
    Entry32    = (UINT32 *)(Rsdt + 1);
    Entry32Num = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
    for (Idx = 0; Idx < Entry32Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry32[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Rsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  //
  // Search Xsdt Second
  //
  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Rsdp->XsdtAddress);
  if (Xsdt != NULL) {
    Entry64    = (UINT64 *)(Xsdt + 1);
    Entry64Num = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 3;
    for (Idx = 0; Idx < Entry64Num; Idx++) {
      Signature = (UINT32 *)(UINTN)Entry64[Idx];
      if (*Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Fadt = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
        DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      }

      if (*Signature == EFI_ACPI_5_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE) {
        MmCfgHdr = (EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER *)Signature;
        DEBUG ((DEBUG_INFO, "Found MM config address in Xsdt\n"));
      }

      if ((Fadt != NULL) && (MmCfgHdr != NULL)) {
        goto Done;
      }
    }
  }

  if (Fadt == NULL) {
    return RETURN_NOT_FOUND;
  }

Done:

  AcpiBoardInfo->PmCtrlRegBase   = Fadt->Pm1aCntBlk;
  AcpiBoardInfo->PmTimerRegBase  = Fadt->PmTmrBlk;
  AcpiBoardInfo->ResetRegAddress = Fadt->ResetReg.Address;
  AcpiBoardInfo->ResetValue      = Fadt->ResetValue;
  AcpiBoardInfo->PmEvtBase       = Fadt->Pm1aEvtBlk;
  AcpiBoardInfo->PmGpeEnBase     = Fadt->Gpe0Blk + Fadt->Gpe0BlkLen / 2;

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
