/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PeiPeCoffLoader.c
    
Abstract:

  GUID for the PE/COFF Loader APIs shared between PEI and DXE

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PeiPeCoffLoader)

EFI_GUID gEfiPeiPeCoffLoaderGuid  = EFI_PEI_PE_COFF_LOADER_GUID;

EFI_GUID_STRING(&gEfiPeiPeCoffLoaderGuid, "PE/COFF Loader", "PE/COFF Loader APIs from PEI");

