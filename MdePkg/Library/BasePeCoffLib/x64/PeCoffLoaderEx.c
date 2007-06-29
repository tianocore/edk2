/** @file
  x64 Specific relocation fixups.

Copyright (c) 2005 - 2006 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "BasePeCoffLibInternals.h"

/**
  Performs an x64 specific relocation fixup.

  @param  Reloc       Pointer to the relocation record
  @param  Fixup       Pointer to the address to fix up
  @param  FixupData   Pointer to a buffer to log the fixups
  @param  Adjust      The offset to adjust the fixup

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
  return RETURN_UNSUPPORTED;
}

/**
  Returns TRUE if the machine type of PE/COFF image is supported. Supported 
  does not mean the image can be executed it means the PE/COFF loader supports
  loading and relocating of the image type. It's up to the caller to support
  the entry point. 

  This function implies the basic PE/COFF loader/relocator supports IA32, EBC,
  & X64 images. Calling the entry point in a correct mannor is up to the 
  consumer of this library.

  @param  Machine   Machine type from the PE Header.

  @return TRUE if this PE/COFF loader can load the image

**/
BOOLEAN
PeCoffLoaderImageFormatSupported (
  IN  UINT16  Machine
  )
{
  if ((Machine == EFI_IMAGE_MACHINE_IA32) || (Machine == EFI_IMAGE_MACHINE_X64) || 
      (Machine ==  EFI_IMAGE_MACHINE_EBC)) {
    return TRUE; 
  }

  return FALSE;
}


/**
  Performs an X64 specific re-relocation fixup and is a no-op on other
  instruction sets. This is used to re-relocated the image into the EFI virtual
  space for runtime calls.

  @param  Reloc       Pointer to the relocation record.
  @param  Fixup       Pointer to the address to fix up.
  @param  FixupData   Pointer to a buffer to log the fixups.
  @param  Adjust      The offset to adjust the fixup.

  @return Status code.

**/
RETURN_STATUS
PeHotRelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
{
  return RETURN_UNSUPPORTED;
}
