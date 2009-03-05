/** @file
  Null PE/Coff Extra Action library instances with empty functions.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/PeCoffExtraActionLib.h>
/**
  Applies additional actions to relocate fixups to a PE/COFF image.

  Generally this function is called after sucessfully Applying relocation fixups 
  to a PE/COFF image for some specicial purpose. 
  As a example, For NT32 emulator, the function should be implemented and called
  to support source level debug.  
  
  @param  ImageContext        Pointer to the image context structure that describes the PE/COFF
                              image that is being relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
}  

/**
  Unloads a loaded PE/COFF image from memory and releases its taken resource.
  
  Releases any environment specific resources that were allocated when the image 
  specified by ImageContext was loaded using PeCoffLoaderLoadImage(). 
  For NT32 emulator, the PE/COFF image loaded by system needs to release.
  For real platform, the PE/COFF image loaded by Core doesn't needs to be unloaded, 
  
  If ImageContext is NULL, then ASSERT().
  
  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image to be unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
}
