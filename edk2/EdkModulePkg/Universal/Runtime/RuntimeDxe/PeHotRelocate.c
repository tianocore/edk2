/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeHotRelocate.c

Abstract:


--*/

#include "Runtime.h"

STATIC
VOID *
RuntimePeImageAddress (
  IN   RUNTIME_IMAGE_RELOCATION_DATA  *Image,
  IN   UINTN                          Address
  )
/*++

Routine Description:

  Converts an image address to the loaded address

Arguments:

  Image         - The relocation data of the image being loaded

  Address       - The address to be converted to the loaded address

Returns:

  NULL if the address can not be converted, otherwise, the converted address

--*/
{
  if (Address >= (Image->ImageSize) << EFI_PAGE_SHIFT) {
    return NULL;
  }

  return (CHAR8 *) ((UINTN) Image->ImageBase + Address);
}

