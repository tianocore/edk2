/** @file
  Entry point of OVMF ACPI Platform Driver

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Protocol/PciEnumerationComplete.h>
#include "AcpiPlatform.h"

STATIC
EFI_ACPI_TABLE_PROTOCOL *
FindAcpiTableProtocol (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID**)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  return AcpiTable;
}


STATIC
VOID
EFIAPI
OnPciEnumerated (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS Status;

  DEBUG ((EFI_D_INFO, "%a: PCI enumeration complete, installing ACPI tables\n",
    __FUNCTION__));
  Status = InstallAcpiTables (FindAcpiTableProtocol ());
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a: InstallAcpiTables: %r\n", __FUNCTION__, Status));
  }
  gBS->CloseEvent (Event);
}


EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  VOID       *Interface;
  EFI_EVENT  PciEnumerated;
  VOID       *Registration;

  //
  // If the platform doesn't support PCI, or PCI enumeration has been disabled,
  // install the tables at once, and let the entry point's return code reflect
  // the full functionality.
  //
  if (PcdGetBool (PcdPciDisableBusEnumeration)) {
    DEBUG ((EFI_D_INFO, "%a: PCI or its enumeration disabled, installing "
      "ACPI tables\n", __FUNCTION__));
    return InstallAcpiTables (FindAcpiTableProtocol ());
  }

  //
  // Similarly, if PCI enumeration has already completed, install the tables
  // immediately.
  //
  Status = gBS->LocateProtocol (&gEfiPciEnumerationCompleteProtocolGuid,
                  NULL /* Registration */, &Interface);
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "%a: PCI enumeration already complete, "
      "installing ACPI tables\n", __FUNCTION__));
    return InstallAcpiTables (FindAcpiTableProtocol ());
  }
  ASSERT (Status == EFI_NOT_FOUND);

  //
  // Otherwise, delay installing the ACPI tables until PCI enumeration
  // completes. The entry point's return status will only reflect the callback
  // setup.
  //
  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, OnPciEnumerated,
                  NULL /* Context */, &PciEnumerated);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiPciEnumerationCompleteProtocolGuid, PciEnumerated,
                  &Registration);
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (PciEnumerated);
  } else {
    DEBUG ((EFI_D_INFO, "%a: PCI enumeration pending, registered callback\n",
      __FUNCTION__));
  }

  return Status;
}
