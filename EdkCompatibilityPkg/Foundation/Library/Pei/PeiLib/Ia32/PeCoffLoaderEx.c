/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeCoffLoaderEx.c

Abstract:

    IA-32 Specific relocation fixups

Revision History

--*/

#include "TianoCommon.h"
#include "EfiImage.h"

EFI_STATUS
PeCoffLoaderRelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
/*++

Routine Description:

  Performs an IA-32 specific relocation fixup

Arguments:

  Reloc      - Pointer to the relocation record

  Fixup      - Pointer to the address to fix up

  FixupData  - Pointer to a buffer to log the fixups

  Adjust     - The offset to adjust the fixup

Returns:

  EFI_UNSUPPORTED   - relocate unsupported

--*/
{
  return EFI_UNSUPPORTED;
}

BOOLEAN
PeCoffLoaderImageFormatSupported (
  IN  UINT16  Machine
  )
/*++
Routine Description:

  Returns TRUE if the machine type of PE/COFF image is supported. Supported 
  does not mean the image can be executed it means the PE/COFF loader supports
  loading and relocating of the image type. It's up to the caller to support
  the entry point. 

  This function implies the basic PE/COFF loader/relocator supports IA32, EBC,
  & X64 images. Calling the entry point in a correct mannor is up to the 
  consumer of this library.

Arguments:

  Machine   - Machine type from the PE Header.

Returns:

  TRUE      - if this PE/COFF loader can load the image
  FALSE     - if this PE/COFF loader cannot load the image

--*/
{
  if ((Machine == EFI_IMAGE_MACHINE_IA32) || (Machine == EFI_IMAGE_MACHINE_X64) || 
      (Machine ==  EFI_IMAGE_MACHINE_EBC)) {
    return TRUE; 
  }

  return FALSE;
}

