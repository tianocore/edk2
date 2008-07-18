/** @file
  This module loads an image to memory for IA32 Cpu architecture.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeIpl.h"

/**
  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

  @param  FileHandle   The handle to the PE/COFF file 
  @param  FileOffset   The offset, in bytes, into the file to read 
  @param  ReadSize     The number of bytes to read from the file starting at 
                       FileOffset 
  @param  Buffer       A pointer to the buffer to read the data into. 

  @retval EFI_SUCCESS  ReadSize bytes of data were read into Buffer from the 
                       PE/COFF file starting at FileOffset

**/
EFI_STATUS
PeiImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
{
  UINT8 *Destination32;
  UINT8 *Source32;
  UINTN  Length;

 
  Destination32 = Buffer;
  Source32      = (UINT8 *) ((UINTN) FileHandle + FileOffset);

  //
  // This function assumes 32-bit alignment to increase performance
  //

  Length = *ReadSize;
  while (Length-- != 0) {
    *(Destination32++) = *(Source32++);
  }

  return EFI_SUCCESS;
}


/**
   This function simply retrieves the function pointer of ImageRead in
   ImageContext structure.
    
   @param ImageContext       A pointer to the structure of 
                             PE_COFF_LOADER_IMAGE_CONTEXT
   
   @retval EFI_SUCCESS       This function always returns EFI_SUCCESS.

**/
EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  VOID        *MemoryBuffer;

  if (gInMemory) {
    ImageContext->ImageRead = PeiImageRead;
    return EFI_SUCCESS;
  }

  //
  // BugBug; This code assumes PeiImageRead() is less than a page in size!
  //  Allocate a page so we can shaddow the read function from FLASH into 
  //  memory to increase performance. 
  //
  
  MemoryBuffer = AllocateCopyPool (0x400, (VOID *)(UINTN) PeiImageRead);
  ASSERT (MemoryBuffer != NULL);

  ImageContext->ImageRead = (PE_COFF_LOADER_READ_FILE) (UINTN) MemoryBuffer;

  return EFI_SUCCESS;
}
