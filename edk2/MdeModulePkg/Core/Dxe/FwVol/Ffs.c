/** @file
  FFS file access utilities.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <DxeMain.h>


/**
  Get the FFS file state by checking the highest bit set in the header's state field.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header

  @return FFS File state

**/
EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
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

  return (EFI_FFS_FILE_STATE) HighestBit;
}



/**
  Check if a block of buffer is erased.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  InBuffer       The buffer to be checked
  @param  BufferSize     Size of the buffer in bytes

  @retval TRUE           The block of buffer is erased
  @retval FALSE          The block of buffer is not erased

**/
BOOLEAN
IsBufferErased (
  IN UINT8    ErasePolarity,
  IN VOID     *InBuffer,
  IN UINTN    BufferSize
  )
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



/**
  Verify checksum of the firmware volume header.

  @param  FvHeader       Points to the firmware volume header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyFvHeaderChecksum (
  IN EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  )
{
  UINT32  Index;
  UINT32  HeaderLength;
  UINT16  Checksum;
  UINT16  *Ptr;

  HeaderLength = FvHeader->HeaderLength;
  Ptr = (UINT16 *)FvHeader;
  Checksum = 0;

  for (Index = 0; Index < HeaderLength / sizeof (UINT16); Index++) {
    Checksum = (UINT16)(Checksum + Ptr[Index]);
  }

  if (Checksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}


/**
  Verify checksum of the FFS file header.

  @param  FfsHeader      Points to the FFS file header to be checked

  @retval TRUE           Checksum verification passed
  @retval FALSE          Checksum verification failed

**/
BOOLEAN
VerifyHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  UINT32            Index;
  UINT8             *Ptr;
  UINT8             HeaderChecksum;

  Ptr = (UINT8 *)FfsHeader;
  HeaderChecksum = 0;
  for (Index = 0; Index < sizeof(EFI_FFS_FILE_HEADER); Index++) {
    HeaderChecksum = (UINT8)(HeaderChecksum + Ptr[Index]);
  }

  HeaderChecksum = (UINT8) (HeaderChecksum - FfsHeader->State - FfsHeader->IntegrityCheck.Checksum.File);

  if (HeaderChecksum == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}



/**
  Check if it's a valid FFS file header.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file header to be checked
  @param  FileState      FFS file state to be returned

  @retval TRUE           Valid FFS file header
  @retval FALSE          Invalid FFS file header

**/
BOOLEAN
IsValidFfsHeader (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  OUT EFI_FFS_FILE_STATE  *FileState
  )
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


/**
  Check if it's a valid FFS file.
  Here we are sure that it has a valid FFS file header since we must call IsValidFfsHeader() first.

  @param  ErasePolarity  Erase polarity attribute of the firmware volume
  @param  FfsHeader      Points to the FFS file to be checked

  @retval TRUE           Valid FFS file
  @retval FALSE          Invalid FFS file

**/
BOOLEAN
IsValidFfsFile (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
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


