/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  AlternateFvBlock.h
    
Abstract:

  Tiano Guid used to define the Alternate Firmware Volume Block Guid.  

--*/

#ifndef _ALT_FVB_GUID_H
#define _ALT_FVB_GUID_H

#define EFI_ALTERNATE_FV_BLOCK_GUID \
  { \
    0xf496922d, 0x172f, 0x4bbc, {0xa1, 0xeb, 0xe, 0xeb, 0x94, 0x9c, 0x34, 0x86} \
  }

extern EFI_GUID gEfiAlternateFvBlockGuid;

#endif
