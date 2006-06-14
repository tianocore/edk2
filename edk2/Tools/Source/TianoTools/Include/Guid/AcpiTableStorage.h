/** @file
  The ACPI table storage file is fully FFS compliant. 
  The file is a number of sections of type EFI_SECTION_RAW.
  This GUID is used to identify the file as an ACPI table storage file.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  AcpiTableStorage.h

  @par Revision Reference:
  GUID defined in ACPI Table Storage Spec Version 0.9.

**/

#ifndef _ACPI_TABLE_STORAGE_H_
#define _ACPI_TABLE_STORAGE_H_

#define EFI_ACPI_TABLE_STORAGE_GUID \
  { 0x7e374e25, 0x8e01, 0x4fee, {0x87, 0xf2, 0x39, 0xc, 0x23, 0xc6, 0x6, 0xcd } }

extern EFI_GUID gEfiAcpiTableStorageGuid;

#endif
