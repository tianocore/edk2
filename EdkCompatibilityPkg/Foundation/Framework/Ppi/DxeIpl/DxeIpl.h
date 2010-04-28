/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 DxeIpl.h

Abstract:

  DXE Initial Program Load PPI as defined in Tiano

  When the PEI core is done it calls the DXE IPL via this PPI.

--*/

#ifndef _DXE_IPL_H_
#define _DXE_IPL_H_

#include "Tiano.h"
#include "PeiHob.h"

#define EFI_DXE_IPL_PPI_GUID \
  { \
    0xae8ce5d, 0xe448, 0x4437, {0xa8, 0xd7, 0xeb, 0xf5, 0xf1, 0x94, 0xf7, 0x31} \
  }

EFI_FORWARD_DECLARATION (EFI_DXE_IPL_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_DXE_IPL_ENTRY) (
  IN EFI_DXE_IPL_PPI              * This,
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_HOB_POINTERS         HobList
  );

struct _EFI_DXE_IPL_PPI {
  EFI_DXE_IPL_ENTRY Entry;
};

extern EFI_GUID gEfiDxeIplPpiGuid;

#endif
