/** @file
  OVMF ACPI Platform Driver for Xen guests

  Copyright (C) 2021, Red Hat, Inc.
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AcpiPlatformLib.h> // InstallAcpiTablesFromMemory()
#include <Library/DebugLib.h>        // DEBUG()
#include <Library/XenPlatformLib.h>  // XenDetected()

#include "AcpiPlatform.h"

#define XEN_ACPI_PHYSICAL_ADDRESS  0x000EA020
#define XEN_BIOS_PHYSICAL_END      0x000FFFFF

/**
  Effective entrypoint of Acpi Platform driver.

  @param  ImageHandle
  @param  SystemTable

  @return EFI_SUCCESS
  @return EFI_LOAD_ERROR
  @return EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
EFIAPI
InstallAcpiTables (
  IN   EFI_ACPI_TABLE_PROTOCOL  *AcpiTable
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *RsdpStructurePtr;
  EFI_XEN_INFO                                  *XenInfo;
  EFI_STATUS                                    Status;

  if (XenDetected ()) {
    //
    // Detect the RSDP structure
    //

    //
    // First look for PVH one
    //
    XenInfo = XenGetInfoHOB ();
    ASSERT (XenInfo != NULL);
    if (XenInfo->RsdpPvh != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "%a: Use ACPI RSDP table at 0x%p\n",
        gEfiCallerBaseName,
        XenInfo->RsdpPvh
        ));
      RsdpStructurePtr = XenInfo->RsdpPvh;
    } else {
      //
      // Otherwise, look for the HVM one
      //
      Status = GetAcpiRsdpFromMemory (
                 XEN_ACPI_PHYSICAL_ADDRESS,
                 XEN_BIOS_PHYSICAL_END,
                 &RsdpStructurePtr
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    Status = InstallAcpiTablesFromRsdp (
               AcpiTable,
               RsdpStructurePtr
               );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}
