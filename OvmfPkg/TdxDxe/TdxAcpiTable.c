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
#include <Protocol/Cpu.h>
#include <Uefi.h>
#include <TdxAcpiTable.h>

/**
  Alter the MADT when ACPI Table from QEMU is available.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
AlterAcpiTable (
  IN EFI_EVENT                      Event,
  IN VOID*                          Context
  )
{
  EFI_ACPI_SDT_PROTOCOL          *AcpiSdtProtocol;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTableProtocol;
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_ACPI_SDT_HEADER            *Table;
  EFI_ACPI_TABLE_VERSION         Version;
  UINTN                          OriginalTableKey;
  UINTN                          NewTableKey;
  UINT8                          *NewMadtTable;
  UINTN                          NewMadtTableLength;
  EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE        *MadtMpWk;
  EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *MadtHeader;

  Index         = 0;
  NewMadtTable  = NULL;
  MadtHeader    = NULL;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (void **) &AcpiSdtProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to locate ACPI SDT protocol.\n"));
    return;
  }

  do {
    Status = AcpiSdtProtocol->GetAcpiTable (Index, &Table, &Version, &OriginalTableKey);

    if (!EFI_ERROR (Status) && Table->Signature == EFI_ACPI_1_0_APIC_SIGNATURE) {
      Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (void **) &AcpiTableProtocol);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to locate ACPI Table protocol.\n"));
        break;
      }

      NewMadtTableLength  = Table->Length + sizeof (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE);
      NewMadtTable        = AllocatePool (NewMadtTableLength);
      if (NewMadtTable == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: OUT_OF_SOURCES error.\n", __FUNCTION__));
        break;
      }

      CopyMem (NewMadtTable, (UINT8 *)Table, Table->Length);
      MadtHeader                = (EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)NewMadtTable;
      MadtHeader->Header.Length = (UINT32)NewMadtTableLength;

      MadtMpWk                  = (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE *)(NewMadtTable + Table->Length);
      MadtMpWk->Type            = EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP;
      MadtMpWk->Length          = sizeof (EFI_ACPI_6_4_MULTIPROCESSOR_WAKEUP_STRUCTURE);
      MadtMpWk->MailBoxVersion  = 1;
      MadtMpWk->Reserved        = 0;
      MadtMpWk->MailBoxAddress  = PcdGet64 (PcdTdRelocatedMailboxBase);

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
    Index ++;
  } while (!EFI_ERROR (Status));

  if (NewMadtTable != NULL) {
    FreePool (NewMadtTable);
  }
}
