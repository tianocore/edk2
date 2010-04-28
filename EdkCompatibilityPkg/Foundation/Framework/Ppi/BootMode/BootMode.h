/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootMode.h
    
Abstract:

  Boot Mode PPI as defined in Tiano

--*/

#ifndef _PEI_MASTER_BOOT_MODE_PPI_H
#define _PEI_MASTER_BOOT_MODE_PPI_H

#define PEI_MASTER_BOOT_MODE_PEIM_PPI \
  { \
    0x7408d748, 0xfc8c, 0x4ee6, {0x92, 0x88, 0xc4, 0xbe, 0xc0, 0x92, 0xa4, 0x10} \
  }

EFI_FORWARD_DECLARATION (PEI_MASTER_BOOT_MODE_PPI);

extern EFI_GUID gPeiMasterBootModePpiGuid;

#endif
