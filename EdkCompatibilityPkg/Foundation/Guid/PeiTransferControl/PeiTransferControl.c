/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiTransferControl.c
    
Abstract:

  GUID for the SetJump()/LongJump() APIs shared between PEI and DXE

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PeiTransferControl)

EFI_GUID gEfiPeiTransferControlGuid = EFI_PEI_TRANSFER_CONTROL_GUID;

EFI_GUID_STRING(&gEfiPeiTransferControlGuid, "Transfer Control", "Transfer Control APIs from PEI");

