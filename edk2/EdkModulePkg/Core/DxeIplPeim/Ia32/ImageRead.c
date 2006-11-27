/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ImageRead.c

Abstract:

--*/

#include "DxeIpl.h"

EFI_STATUS
EFIAPI
PeiImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  UINT8 *Destination32;
  UINT8 *Source32;
  UINTN  Length;

 
  Destination32 = Buffer;
  Source32      = (UINT8 *) ((UINTN) FileHandle + FileOffset);

  //
  // This function assumes 32-bit alignment to increase performance
  //
//  ASSERT (ALIGN_POINTER (Destination32, sizeof (UINT32)) == Destination32);
//  ASSERT (ALIGN_POINTER (Source32, sizeof (UINT32)) == Source32);

  Length = *ReadSize;
  while (Length--) {
    *(Destination32++) = *(Source32++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:
  Support routine to return the PE32 Image Reader.
  If the PeiImageRead() function is less than a page
  in legnth. If the function is more than a page the DXE IPL will crash!!!!

Arguments:
  ImageContext  - The context of the image being loaded

Returns:
  EFI_SUCCESS - If Image function location is found

--*/
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
