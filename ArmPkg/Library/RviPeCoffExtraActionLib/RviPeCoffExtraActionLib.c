/**@file

Copyright (c) 2006 - 2009, Intel Corporation
Portions copyright (c) 2008-2010 Apple Inc. All rights reserved.
All rights reserved. This program and the accompanying materials
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
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>


VOID
DeCygwinIfNeeded (
  IN  CHAR8   *Name
  )
{
  CHAR8   *Ptr;
  UINTN   Index;
  UINTN   Len;
  
  Ptr = AsciiStrStr (Name, "/cygdrive/");
  if (Ptr == NULL) {
    return;
  }
  
  Len = AsciiStrLen (Ptr);
  
  // convert "/cygdrive" to spaces
  for (Index = 0; Index < 9; Index++) {
    Ptr[Index] = ' ';
  }

  // convert /c to c:
  Ptr[9]  = Ptr[10];
  Ptr[10] = ':';
  
  // switch path seperators
  for (Index = 11; Index < Len; Index++) {
    if (Ptr[Index] == '/') {
      Ptr[Index] = '\\' ;
    }
  }
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
  CHAR8 Buffer[256];
  
  AsciiSPrint (Buffer, sizeof(Buffer), "load /a /ni /np %a &0x%08x\n", ImageContext->PdbPointer, (UINTN)(ImageContext->ImageAddress + ImageContext->SizeOfHeaders));
  DeCygwinIfNeeded (&Buffer[16]);
 
  SerialPortWrite ((UINT8 *) Buffer, AsciiStrLen (Buffer));
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
  CHAR8 Buffer[256];
  
  AsciiSPrint (Buffer, sizeof(Buffer), "unload symbols_only %a", ImageContext->PdbPointer);
  DeCygwinIfNeeded (Buffer);
 
  SerialPortWrite ((UINT8 *) Buffer, AsciiStrLen (Buffer));
}
