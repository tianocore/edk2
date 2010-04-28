/** @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PpisNeededByDxeCore.h

Abstract:
  
Revision History:

**/

#ifndef _DXELDR_PPIS_NEEDED_BY_DXE_CORE_H_
#define _DXELDR_PPIS_NEEDED_BY_DXE_CORE_H_

#include "DxeIpl.h"
#include "HobGeneration.h"

//EFI_STATUS
//InstallEfiPeiTransferControl (
//  IN OUT EFI_PEI_TRANSFER_CONTROL_PROTOCOL **This
//  );

//EFI_STATUS
//InstallEfiPeiFlushInstructionCache (
//  IN OUT EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  **This
//  );

EFI_STATUS
EFIAPI
PreparePpisNeededByDxeCore (
  IN  HOB_TEMPLATE  *HobStart
  )
/*++

Routine Description:

  This routine adds the PPI/Protocol Hobs that are consumed by the DXE Core.
  Normally these come from PEI, but since our PEI was 32-bit we need an
  alternate source. That is this driver.

  This driver does not consume PEI or DXE services and thus updates the 
  Phit (HOB list) directly

Arguments:

  HobStart - Pointer to the beginning of the HOB List from PEI

Returns:

  This function should after it has add it's HOBs

--*/
;

#endif
