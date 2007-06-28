/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Acpi.h
    
Abstract:

  GUIDs used for ACPI entries in the in the EFI 1.0 system table

  These GUIDs point the ACPI tables as defined in the ACPI specifications.
  ACPI 2.0 specification defines the ACPI 2.0 GUID. EFI 1.0 defines the 
  ACPI 1.0 GUID.

--*/

#ifndef _ACPI_GUID_H_
#define _ACPI_GUID_H_

#define EFI_ACPI_TABLE_GUID \
  { \
    0xeb9d2d30, 0x2d88, 0x11d3, 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d \
  }

#define EFI_ACPI_20_TABLE_GUID \
  { \
    0x8868e871, 0xe4f1, 0x11d3, 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 \
  }

#define EFI_ACPI_30_TABLE_GUID EFI_ACPI_20_TABLE_GUID

extern EFI_GUID gEfiAcpiTableGuid;
extern EFI_GUID gEfiAcpi20TableGuid;
extern EFI_GUID gEfiAcpi30TableGuid;
#endif
