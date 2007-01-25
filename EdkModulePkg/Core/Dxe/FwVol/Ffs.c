/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ffs.c

Abstract:

  FFS file access utilities.

--*/


#include <DxeMain.h>

#define PHYSICAL_ADDRESS_TO_POINTER(Address) ((VOID *)((UINTN)(Address)))


EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Get the FFS file state by checking the highest bit set in the header's state field

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file header
    
Returns:
  FFS File state 
    
--*/
{
  EFI_FFS_FILE_STATE      FileState;
  UINT8                   HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE)~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && ((HighestBit & FileState) == 0)) {
    HighestBit >>= 1;
  }

  return (EFI_FFS_FILE_STATE)HighestBit;
}


BOOLEAN
IsBufferErased (
  IN UINT8    ErasePolarity,
  IN VOID     *InBuffer,
  IN UINTN    BufferSize
  )
/*++

Routine Description:
  Check if a block of buffer is erased

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  InBuffer      -  The buffer to be checked
  BufferSize    -  Size of the buffer in bytes
    
Returns:
  TRUE  -  The block of buffer is erased
  FALSE -  The block of buffer is not erased
    
--*/
{
  UINTN   Count;
  UINT8   EraseByte;
  UINT8   *Buffer;

  if(ErasePolarity == 1) {
    EraseByte = 0xFF;
  } else {
    EraseByte = 0;
  }

  Buffer = InBuffer;
  for (Count = 0; Count < BufferSize; Count++) {
    if (Buffer[Count] != EraseByte) {
      return FALSE;
    }
  }

  return TRUE;
}


BOOLEAN
VerifyFvHeaderChecksum (
  IN EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  )
/*++

Routine Description:
  Verify checksum of the firmware volume header 

Arguments:
  FvHeader  -  Points to the firmware volume header to be checked
    
Returns:
  TRUE  -  Checksum verification passed
  FALSE -  Checksum verification failed
    
--*/
{
  UINT32  Index;
  UINT32  HeaderLength;
  UINT16  Checksum;
  UINT16  *ptr;

  HeaderLength = FvHeader->HeaderLength;
  ptr = (UINT16 *)FvHeader;
  Checksum = 0;

  for (Index = 0; Index < HeaderLength / sizeof (UINT16); Index++) {
    Checksum = (UINT16)(Checksum + ptr[Index]);
  }

  if (Checksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

STATIC
BOOLEAN
VerifyHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Verify checksum of the FFS file header 

Arguments:
  FfsHeader  -  Points to the FFS file header to be checked
    
Returns:
  TRUE  -  Checksum verification passed
  FALSE -  Checksum verification failed
    
--*/
{
  UINT32            Index;
  UINT8             *ptr;
  UINT8             HeaderChecksum;

  ptr = (UINT8 *)FfsHeader;
  HeaderChecksum = 0;
  for (Index = 0; Index < sizeof(EFI_FFS_FILE_HEADER); Index++) {
    HeaderChecksum = (UINT8)(HeaderChecksum + ptr[Index]);
  }

  HeaderChecksum = (UINT8) (HeaderChecksum - FfsHeader->State - FfsHeader->IntegrityCheck.Checksum.File);

  if (HeaderChecksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}


BOOLEAN
IsValidFfsHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  OUT EFI_FFS_FILE_STATE  *FileState
  )
/*++

Routine Description:
  Check if it's a valid FFS file header

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file header to be checked
  FileState     -  FFS file state to be returned
    
Returns:
  TRUE  -  Valid FFS file header
  FALSE -  Invalid FFS file header
    
--*/
{
  *FileState = GetFileState (ErasePolarity, FfsHeader);

  switch (*FileState) {
    case EFI_FILE_HEADER_VALID:
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
    case EFI_FILE_DELETED:
      //
      // Here we need to verify header checksum
      //
      return VerifyHeaderChecksum (FfsHeader);
    
    case EFI_FILE_HEADER_CONSTRUCTION:
    case EFI_FILE_HEADER_INVALID:
    default:
      return FALSE;
  }
}


BOOLEAN
IsValidFfsFile (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:
  Check if it's a valid FFS file. 
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

Arguments:
  ErasePolarity -  Erase polarity attribute of the firmware volume
  FfsHeader     -  Points to the FFS file to be checked
    
Returns:
  TRUE  -  Valid FFS file
  FALSE -  Invalid FFS file
    
--*/
{
  EFI_FFS_FILE_STATE  FileState;

  FileState = GetFileState (ErasePolarity, FfsHeader);
  switch (FileState) {

    case EFI_FILE_DELETED:
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      //
      // Some other vliadation like file content checksum might be done here.
      // For performance issue, Tiano only do FileState check.
      //
      return TRUE;

    default:
      return FALSE;
  }
}

