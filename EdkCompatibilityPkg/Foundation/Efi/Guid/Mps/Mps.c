/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Mps.c
    
Abstract:

  GUIDs used for MPS entries in the in the EFI 1.0 system table

  These GUIDs point the MPS tables as defined in the MPS 1.4 specifications.

  ACPI is the primary means of exporting MP information to the OS. MPS obly was
  included to support Itanium-based platform power on. So don't use it if you don't have too.


--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (Mps)

EFI_GUID  gEfiMpsTableGuid = EFI_MPS_TABLE_GUID;

EFI_GUID_STRING(&gEfiMpsTableGuid, "MPS Table", "MPS Table GUID in EFI System Table");
