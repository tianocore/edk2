/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PcAnsi.c
    
Abstract:

  Terminal Device Path Vendor Guid. Defined in EFI 1.0.

--*/

#include "EfiSpec.h"

#include EFI_GUID_DEFINITION (PcAnsi)

EFI_GUID  gEfiPcAnsiGuid    = EFI_PC_ANSI_GUID;
EFI_GUID  gEfiVT100Guid     = EFI_VT_100_GUID;
EFI_GUID  gEfiVT100PlusGuid = EFI_VT_100_PLUS_GUID;
EFI_GUID  gEfiVTUTF8Guid    = EFI_VT_UTF8_GUID;

EFI_GUID_STRING(&gEfiPcAnsiGuid, "Efi", "Efi PC ANSI Device Path Vendor GUID")
EFI_GUID_STRING(&gEfiVT100Guid, "Efi", "Efi VT100 Device Path Vendor GUID")
EFI_GUID_STRING(&gEfiVT100PlusGuid, "Efi", "Efi VT100Plus Device Path Vendor GUID")
EFI_GUID_STRING(&gEfiVTUTF8Guid, "Efi", "Efi VTUTF8 Device Path Vendor GUID")
