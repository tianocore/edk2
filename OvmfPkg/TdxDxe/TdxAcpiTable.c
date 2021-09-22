/** @file
  OVMF ACPI QEMU support

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>

  Copyright (C) 2012-2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/OrderedCollectionLib.h>
#include <Library/TdxLib.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/TdxMailboxLib.h>
#include <Protocol/Cpu.h>
#include <Uefi.h>
#include <TdxAcpiTable.h>

/**
  At the beginning of system boot, a 4K-aligned, 4K-size memory (Td mailbox) is
  pre-allocated by host VMM. BSP & APs do the page accept together in that memory
  region.

  After that TDVF is designed to relocate the mailbox to a 4K-aligned, 4K-size
  memory block which is allocated in the ACPI Nvs memory. APs are waken up and
  spin around the relocated mailbox for further command.

  @return   EFI_PHYSICAL_ADDRESS    Address of the relocated mailbox
**/
EFI_PHYSICAL_ADDRESS
EFIAPI
RelocateMailbox (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  Address;
  VOID                  *ApLoopFunc;
  UINT32                RelocationPages;
  MP_RELOCATION_MAP     RelocationMap;
  MP_WAKEUP_MAILBOX     *RelocatedMailBox;
  EFI_STATUS            Status;

  Address    = 0;
  ApLoopFunc = NULL;
  ZeroMem (&RelocationMap, sizeof (RelocationMap));

  //
  // Get information needed to setup aps running in their
  // run loop in allocated acpi reserved memory
  // Add another page for mailbox
  //
  AsmGetRelocationMap (&RelocationMap);
  if ((RelocationMap.RelocateApLoopFuncAddress == 0) || (RelocationMap.RelocateApLoopFuncSize == 0)) {
    DEBUG ((DEBUG_ERROR, "Failed to get the RelocationMap.\n"));
    return 0;
  }

  RelocationPages = EFI_SIZE_TO_PAGES ((UINT32)RelocationMap.RelocateApLoopFuncSize) + 1;

  Status = gBS->AllocatePages (AllocateAnyPages, EfiACPIMemoryNVS, RelocationPages, &Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages for MailboxRelocation. %r\n", Status));
    return 0;
  }

  ZeroMem ((VOID *)Address, EFI_PAGES_TO_SIZE (RelocationPages));

  ApLoopFunc = (VOID *)((UINTN)Address + EFI_PAGE_SIZE);

  CopyMem (
    ApLoopFunc,
    RelocationMap.RelocateApLoopFuncAddress,
    RelocationMap.RelocateApLoopFuncSize
    );

  DEBUG ((
    DEBUG_INFO,
    "Ap Relocation: mailbox %llx, loop %p\n",
    Address,
    ApLoopFunc
    ));

  //
  // Initialize mailbox
  //
  RelocatedMailBox               = (MP_WAKEUP_MAILBOX *)Address;
  RelocatedMailBox->Command      = MpProtectedModeWakeupCommandNoop;
  RelocatedMailBox->ApicId       = MP_CPU_PROTECTED_MODE_MAILBOX_APICID_INVALID;
  RelocatedMailBox->WakeUpVector = 0;

  //
  // Wakup APs and have been move to the finalized run loop
  // They will spin until guest OS wakes them
  //
  MpSerializeStart ();

  MpSendWakeupCommand (
    MpProtectedModeWakeupCommandWakeup,
    (UINT64)ApLoopFunc,
    (UINT64)RelocatedMailBox,
    0,
    0,
    0
    );

  return Address;
}

/**
  Alter the MADT when ACPI Table from QEMU is available.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
AlterAcpiTable (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_ACPI_SDT_PROTOCOL                                *AcpiSdtProtocol;
  EFI_ACPI_TABLE_PROTOCOL                              *AcpiTableProtocol;
  EFI_STATUS                                           Status;
  UINTN                                                Index;
  EFI_ACPI_SDT_HEADER                                  *Table;
  EFI_ACPI_TABLE_VERSION                               Version;
  UINTN                                                OriginalTableKey;
  UINTN                                                NewTableKey;
  UINT8                                                *NewMadtTable;
  UINTN                                                NewMadtTableLength;
  EFI_PHYSICAL_ADDRESS                                 RelocateMailboxAddress;
  EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE         *MadtMpWk;
  EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *MadtHeader;

  Index        = 0;
  NewMadtTable = NULL;
  MadtHeader   = NULL;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (void **)&AcpiSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI SDT protocol.\n"));
    return;
  }

  RelocateMailboxAddress = RelocateMailbox ();
  if (RelocateMailboxAddress == 0) {
    ASSERT (FALSE);
    DEBUG ((DEBUG_ERROR, "Failed to relocate Td mailbox\n"));
    return;
  }

  do {
    Status = AcpiSdtProtocol->GetAcpiTable (Index, &Table, &Version, &OriginalTableKey);

    if (!EFI_ERROR (Status) && (Table->Signature == EFI_ACPI_1_0_APIC_SIGNATURE)) {
      Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (void **)&AcpiTableProtocol);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to locate ACPI Table protocol.\n"));
        break;
      }

      NewMadtTableLength = Table->Length + sizeof (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE);
      NewMadtTable       = AllocatePool (NewMadtTableLength);
      if (NewMadtTable == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: OUT_OF_SOURCES error.\n", __FUNCTION__));
        break;
      }

      CopyMem (NewMadtTable, (UINT8 *)Table, Table->Length);
      MadtHeader                = (EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)NewMadtTable;
      MadtHeader->Header.Length = (UINT32)NewMadtTableLength;

      MadtMpWk                 = (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE *)(NewMadtTable + Table->Length);
      MadtMpWk->Type           = EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP;
      MadtMpWk->Length         = sizeof (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE);
      MadtMpWk->MailBoxVersion = 1;
      MadtMpWk->Reserved       = 0;
      MadtMpWk->MailBoxAddress = RelocateMailboxAddress;

      Status = AcpiTableProtocol->InstallAcpiTable (AcpiTableProtocol, NewMadtTable, NewMadtTableLength, &NewTableKey);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to install new MADT table. %r\n", Status));
        break;
      }

      Status = AcpiTableProtocol->UninstallAcpiTable (AcpiTableProtocol, OriginalTableKey);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Uninstall old MADT table error.\n"));
      }

      break;
    }

    Index++;
  } while (!EFI_ERROR (Status));

  if (NewMadtTable != NULL) {
    FreePool (NewMadtTable);
  }
}
