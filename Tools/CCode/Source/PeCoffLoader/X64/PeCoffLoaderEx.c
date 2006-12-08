/**@file
  x64 Specific relocation fixups.

Copyright (c) 2005 - 2006 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>
#include <Library/PeCoffLib.h>

/**
  Performs an x64 specific relocation fixup

  @param Reloc        Pointer to the relocation record
  @param Fixup        Pointer to the address to fix up
  @param FixupData    Pointer to a buffer to log the fixups
  @param Adjust       The offset to adjust the fixup
  
  @retval RETURN_SUCCESS      Success to perform relocation
  @retval RETURN_UNSUPPORTED  Unsupported.
**/
RETURN_STATUS
PeCoffLoaderRelocateImageEx (
  IN     UINT16       *Reloc,
  IN OUT CHAR8        *Fixup, 
  IN OUT CHAR8        **FixupData,
  IN     UINT64       Adjust
  )
{
  UINT64      *F64;

  switch ((*Reloc) >> 12) {

    case EFI_IMAGE_REL_BASED_DIR64:
      F64 = (UINT64 *) Fixup;
      *F64 = *F64 + (UINT64) Adjust;
      if (*FixupData != NULL) {
        *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
        *(UINT64 *)(*FixupData) = *F64;
        *FixupData = *FixupData + sizeof(UINT64);
      }
      break;

    default:
      return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}
