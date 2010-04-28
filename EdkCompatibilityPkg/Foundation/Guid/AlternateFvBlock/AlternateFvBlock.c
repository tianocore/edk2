/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AlternateFvBlock.c
    
Abstract:

  Tiano Guid used to define the Alternate Firmware Volume Block Guid.  

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (AlternateFvBlock)

EFI_GUID  gEfiAlternateFvBlockGuid = EFI_ALTERNATE_FV_BLOCK_GUID;

EFI_GUID_STRING
  (&gEfiAlternateFvBlockGuid, "Alternate Firmware Volume Block GUID", "Alternate Firmware Volume Block GUID");
