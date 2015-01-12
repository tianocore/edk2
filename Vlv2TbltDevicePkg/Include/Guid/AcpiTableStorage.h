/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


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
