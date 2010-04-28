/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FlashMap.h

Abstract:

  FlashMap PPI defined in Tiano

  This code abstracts FlashMap access

--*/

#ifndef _PEI_FLASH_MAP_PPI_H_
#define _PEI_FLASH_MAP_PPI_H_

#define PEI_FLASH_MAP_PPI_GUID \
  { \
    0xf34c2fa0, 0xde88, 0x4270, {0x84, 0x14, 0x96, 0x12, 0x22, 0xf4, 0x52, 0x1c} \
  }

#include "EfiFlashMap.h"

EFI_FORWARD_DECLARATION (PEI_FLASH_MAP_PPI);

//
// Functions
//
typedef
EFI_STATUS
(EFIAPI *PEI_GET_FLASH_AREA_INFO) (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN PEI_FLASH_MAP_PPI            * This,
  IN  EFI_FLASH_AREA_TYPE         AreaType,
  IN  EFI_GUID                    * AreaTypeGuid,
  OUT UINT32                      *NumEntries,
  OUT EFI_FLASH_SUBAREA_ENTRY     **Entries
  );

//
// PEI_FLASH_MAP_PPI
//
struct _PEI_FLASH_MAP_PPI {
  PEI_GET_FLASH_AREA_INFO GetAreaInfo;
};

extern EFI_GUID gPeiFlashMapPpiGuid;

#endif // _PEI_FLASH_MAP_PPI_H_
