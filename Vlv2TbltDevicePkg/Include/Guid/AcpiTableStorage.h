/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  AcpiTableStorage.h

Abstract:

  GUID for the ACPI Table Storage filename.

  This GUID is defined in the Tiano ACPI Table Storage EPS.

--*/

#ifndef _ACPI_TABLE_STORAGE_H_
#define _ACPI_TABLE_STORAGE_H_

#define EFI_ACPI_TABLE_STORAGE_GUID \
  { 0x7e374e25, 0x8e01, 0x4fee, {0x87, 0xf2, 0x39, 0xc, 0x23, 0xc6, 0x6, 0xcd} }

extern EFI_GUID gEfiAcpiTableStorageGuid;

#endif
