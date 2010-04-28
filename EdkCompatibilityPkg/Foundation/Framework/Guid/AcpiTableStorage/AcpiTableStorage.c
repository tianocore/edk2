/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AcpiTableStorage.c
    
Abstract:

  The filename of the Acpi table storage file.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (AcpiTableStorage)

EFI_GUID gEfiAcpiTableStorageGuid = EFI_ACPI_TABLE_STORAGE_GUID;

EFI_GUID_STRING (&gEfiAcpiTableStorageGuid, "ACPI Table Storage File Name", 
                "Tiano ACPI 2.0 Table Storage file name GUID");
