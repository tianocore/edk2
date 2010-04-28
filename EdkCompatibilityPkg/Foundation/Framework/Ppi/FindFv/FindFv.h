/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 FindFv.h

Abstract:

  FindFv PPI as defined in Tiano

  Used to locate FVs that contain PEIMs in PEI

--*/

#ifndef _FIND_FV_H_
#define _FIND_FV_H_

#include "EfiFirmwareVolumeHeader.h"

#define EFI_FIND_FV_PPI_GUID \
  { \
    0x36164812, 0xa023, 0x44e5, {0xbd, 0x85, 0x5, 0xbf, 0x3c, 0x77, 0x0, 0xaa} \
  }

EFI_FORWARD_DECLARATION (EFI_FIND_FV_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_FIND_FV_FINDFV) (
  IN EFI_FIND_FV_PPI          * This,
  IN EFI_PEI_SERVICES         **PeiServices,
  UINT8                       *FvNumber,
  EFI_FIRMWARE_VOLUME_HEADER  **FVAddress
  );

struct _EFI_FIND_FV_PPI {
  EFI_FIND_FV_FINDFV  FindFv;
};

extern EFI_GUID gEfiFindFvPpiGuid;

#endif
