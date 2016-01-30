/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011 - 2012, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/PeCoffLib.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PrintLib.h>


/**
  If the build is done on cygwin the paths are cygpaths.
  /cygdrive/c/tmp.txt vs c:\tmp.txt so we need to convert
  them to work with RVD commands

  @param  Name  Path to convert if needed

**/
CHAR8 *
DeCygwinPathIfNeeded (
  IN  CHAR8   *Name,
  IN  CHAR8   *Temp,
  IN  UINTN   Size
  )
{
  CHAR8   *Ptr;
  UINTN   Index;
  UINTN   Index2;

  Ptr = AsciiStrStr (Name, "/cygdrive/");
  if (Ptr == NULL) {
    return Name;
  }

  for (Index = 9, Index2 = 0; (Index < (Size + 9)) && (Ptr[Index] != '\0'); Index++, Index2++) {
    Temp[Index2] = Ptr[Index];
    if (Temp[Index2] == '/') {
      Temp[Index2] = '\\' ;
  }

    if (Index2 == 1) {
      Temp[Index2 - 1] = Ptr[Index];
      Temp[Index2] = ':';
    }
  }

  return Temp;
}


/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
#if !defined(MDEPKG_NDEBUG)
  CHAR8 Temp[512];
#endif

  if (ImageContext->PdbPointer) {
#ifdef __CC_ARM
#if (__ARMCC_VERSION < 500000)
    // Print out the command for the RVD debugger to load symbols for this image
    DEBUG ((EFI_D_LOAD | EFI_D_INFO, "load /a /ni /np %a &0x%p\n", DeCygwinPathIfNeeded (ImageContext->PdbPointer, Temp, sizeof (Temp)), (UINTN)(ImageContext->ImageAddress + ImageContext->SizeOfHeaders)));
#else
    // Print out the command for the DS-5 to load symbols for this image
    DEBUG ((EFI_D_LOAD | EFI_D_INFO, "add-symbol-file %a 0x%p\n", DeCygwinPathIfNeeded (ImageContext->PdbPointer, Temp, sizeof (Temp)), (UINTN)(ImageContext->ImageAddress + ImageContext->SizeOfHeaders)));
#endif
#elif __GNUC__
    // This may not work correctly if you generate PE/COFF directlyas then the Offset would not be required
    DEBUG ((EFI_D_LOAD | EFI_D_INFO, "add-symbol-file %a 0x%p\n", DeCygwinPathIfNeeded (ImageContext->PdbPointer, Temp, sizeof (Temp)), (UINTN)(ImageContext->ImageAddress + ImageContext->SizeOfHeaders)));
#else
    DEBUG ((EFI_D_LOAD | EFI_D_INFO, "Loading driver at 0x%11p EntryPoint=0x%11p\n", (VOID *)(UINTN) ImageContext->ImageAddress, FUNCTION_ENTRY_POINT (ImageContext->EntryPoint)));
#endif
  } else {
    DEBUG ((EFI_D_LOAD | EFI_D_INFO, "Loading driver at 0x%11p EntryPoint=0x%11p\n", (VOID *)(UINTN) ImageContext->ImageAddress, FUNCTION_ENTRY_POINT (ImageContext->EntryPoint)));
  }
}



/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
#if !defined(MDEPKG_NDEBUG)
  CHAR8 Temp[512];
#endif

  if (ImageContext->PdbPointer) {
#ifdef __CC_ARM
    // Print out the command for the RVD debugger to load symbols for this image
    DEBUG ((EFI_D_ERROR, "unload symbols_only %a\n", DeCygwinPathIfNeeded (ImageContext->PdbPointer, Temp, sizeof (Temp))));
#elif __GNUC__
    // This may not work correctly if you generate PE/COFF directlyas then the Offset would not be required
    DEBUG ((EFI_D_ERROR, "remove-symbol-file %a 0x%08x\n", DeCygwinPathIfNeeded (ImageContext->PdbPointer, Temp, sizeof (Temp)), (UINTN)(ImageContext->ImageAddress + ImageContext->SizeOfHeaders)));
#else
    DEBUG ((EFI_D_ERROR, "Unloading %a\n", ImageContext->PdbPointer));
#endif
  } else {
    DEBUG ((EFI_D_ERROR, "Unloading driver at 0x%11p\n", (VOID *)(UINTN) ImageContext->ImageAddress));
  }
}
