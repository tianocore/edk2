/*++

Copyright (c) 2006, Intel Corporation. All rights reserved. <BR> 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IsaFloppyBlock.c

Abstract:

  ISA Floppy Driver
  1. Support two types diskette drive  
     1.44M drive and 2.88M drive (and now only support 1.44M)
  2. Support two diskette drives
  3. Use DMA channel 2 to transfer data
  4. Do not use interrupt
  5. Support diskette change line signal and write protect
  
  Implement the Block IO interface

Revision History:

--*/

#include "IsaFloppy.h"

EFI_STATUS
EFIAPI
FdcReset (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                ExtendedVerification
  )
/*++
  
  Routine Description:  Reset the Floppy Logic Drive, call the FddReset function   
  Parameters:
    This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
    ExtendedVerification BOOLEAN: Indicate that the driver may perform a more 
                    exhaustive verification operation of the device during 
                    reset, now this par is ignored in this driver          
  Returns:
    EFI_SUCCESS:      The Floppy Logic Drive is reset
    EFI_DEVICE_ERROR: The Floppy Logic Drive is not functioning correctly 
                      and can not be reset

--*/
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    ExtendedVerification - add argument and description to function comment
{
  FDC_BLK_IO_DEV  *FdcDev;

  //
  // Reset the Floppy Disk Controller
  //
  FdcDev = FDD_BLK_IO_FROM_THIS (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_P_PC_RESET | EFI_PERIPHERAL_REMOVABLE_MEDIA,
    FdcDev->DevicePath
    );

  return FddReset (FdcDev);
}

EFI_STATUS
EFIAPI
FddFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This
  )
/*++
  
  Routine Description:  
  Parameters:
    This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  Returns:
    EFI_SUCCESS:    

--*/
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO:    This - add argument and description to function comment
{
  //
  // Not supported yet
  //
  return EFI_SUCCESS;
}

STATIC
VOID
FddReportStatus (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                Read
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This  - GC_TODO: add argument description
  Read  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  FDC_BLK_IO_DEV  *FdcDev;

  FdcDev = FDD_BLK_IO_FROM_THIS (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE,
    ((Read) ? EFI_P_EC_INPUT_ERROR : EFI_P_EC_OUTPUT_ERROR) | EFI_PERIPHERAL_REMOVABLE_MEDIA,
    FdcDev->DevicePath
    );
}

EFI_STATUS
EFIAPI
FddReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                LBA,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
/*++

  Routine Description:  Read the requested number of blocks from the device   
  Parameters:
    This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
    MediaId UINT32:    The media id that the read request is for    
    LBA EFI_LBA:     The starting logic block address to read from on the device
    BufferSize UINTN:  The size of the Buffer in bytes
    Buffer VOID *:     A pointer to the destination buffer for the data
  Returns:
    EFI_SUCCESS:     The data was read correctly from the device
    EFI_DEVICE_ERROR:The device reported an error while attempting to perform
                     the read operation
    EFI_NO_MEDIA:    There is no media in the device
    EFI_MEDIA_CHANGED:   The MediaId is not for the current media
    EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
    EFI_INVALID_PARAMETER:The read request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 

--*/
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    MediaId - add argument and description to function comment
// GC_TODO:    LBA - add argument and description to function comment
// GC_TODO:    BufferSize - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, LBA, BufferSize, READ, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, TRUE);
  }

  return Status;
}

EFI_STATUS
EFIAPI
FddWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                LBA,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
/*++

  Routine Description:  Write a specified number of blocks to the device   
  Parameters:
    This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
    MediaId UINT32:    The media id that the write request is for   
    LBA EFI_LBA:     The starting logic block address to be written
    BufferSize UINTN:  The size in bytes in Buffer
    Buffer VOID *:     A pointer to the source buffer for the data
  Returns :
    EFI_SUCCESS:     The data were written correctly to the device
    EFI_WRITE_PROTECTED: The device can not be written to 
    EFI_NO_MEDIA:    There is no media in the device
    EFI_MEDIA_CHANGED:   The MediaId is not for the current media
    EFI_DEVICE_ERROR:  The device reported an error while attempting to perform 
                       the write operation 
    EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
    EFI_INVALID_PARAMETER:The write request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 

--*/
// GC_TODO: function comment is missing 'Arguments:'
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    MediaId - add argument and description to function comment
// GC_TODO:    LBA - add argument and description to function comment
// GC_TODO:    BufferSize - add argument and description to function comment
// GC_TODO:    Buffer - add argument and description to function comment
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, LBA, BufferSize, WRITE, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, FALSE);
  }

  return Status;
}

EFI_STATUS
FddReadWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                LBA,
  IN  UINTN                  BufferSize,
  IN  BOOLEAN                Operation,
  OUT VOID                   *Buffer
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This        - GC_TODO: add argument description
  MediaId     - GC_TODO: add argument description
  LBA         - GC_TODO: add argument description
  BufferSize  - GC_TODO: add argument description
  Operation   - GC_TODO: add argument description
  Buffer      - GC_TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_NO_MEDIA - GC_TODO: Add description for return value
  EFI_MEDIA_CHANGED - GC_TODO: Add description for return value
  EFI_WRITE_PROTECTED - GC_TODO: Add description for return value
  EFI_BAD_BUFFER_SIZE - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  EFI_BLOCK_IO_MEDIA  *Media;
  FDC_BLK_IO_DEV      *FdcDev;
  UINTN               BlockSize;
  UINTN               NumberOfBlocks;
  UINTN               BlockCount;
  EFI_STATUS          Status;
  //
  //  EFI_STATUS            CacheStatus;
  //
  EFI_LBA             LBA0;
  UINT8               *Pointer;

  //
  // Get the intrinsic block size
  //
  Media     = This->Media;
  BlockSize = Media->BlockSize;
  FdcDev    = FDD_BLK_IO_FROM_THIS (This);

  if (Operation == WRITE) {
    if (LBA == 0) {
      FdcFreeCache (FdcDev);
    }
  }
  //
  // Check the Parameter is valid
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }
  //
  // Set the drive motor on
  //
  Status = MotorOn (FdcDev);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Check to see if media can be detected
  //
  Status = DetectMedia (FdcDev);
  if (EFI_ERROR (Status)) {
    MotorOff (FdcDev);
    FdcFreeCache (FdcDev);
    return EFI_DEVICE_ERROR;
  }
  //
  // Check to see if media is present
  //
  if (!(Media->MediaPresent)) {
    MotorOff (FdcDev);
    FdcFreeCache (FdcDev);

    /*
    if (FdcDev->Cache) {
      gBS->FreePool (FdcDev->Cache);
      FdcDev->Cache = NULL;
    }
*/
    return EFI_NO_MEDIA;
  }
  //
  // Check to see if media has been changed
  //
  if (MediaId != Media->MediaId) {
    MotorOff (FdcDev);
    FdcFreeCache (FdcDev);
    return EFI_MEDIA_CHANGED;
  }

  if (Operation == WRITE) {
    if (Media->ReadOnly) {
      MotorOff (FdcDev);
      return EFI_WRITE_PROTECTED;
    }
  }
  //
  // Check the parameters for this read/write operation
  //
  if (BufferSize % BlockSize != 0) {
    MotorOff (FdcDev);
    return EFI_BAD_BUFFER_SIZE;
  }

  if (LBA > Media->LastBlock) {
    MotorOff (FdcDev);
    return EFI_INVALID_PARAMETER;
  }

  if (((BufferSize / BlockSize) + LBA - 1) > Media->LastBlock) {
    MotorOff (FdcDev);
    return EFI_INVALID_PARAMETER;
  }

  if (Operation == READ) {
    //
    // See if the data that is being read is already in the cache
    //
    if (FdcDev->Cache) {
      if (LBA == 0 && BufferSize == BlockSize) {
        MotorOff (FdcDev);
        CopyMem ((UINT8 *) Buffer, (UINT8 *) FdcDev->Cache, BlockSize);
        return EFI_SUCCESS;
      }
    }
  }
  //
  // Set up Floppy Disk Controller
  //
  Status = Setup (FdcDev);
  if (EFI_ERROR (Status)) {
    MotorOff (FdcDev);
    return EFI_DEVICE_ERROR;
  }

  NumberOfBlocks  = BufferSize / BlockSize;
  LBA0            = LBA;
  Pointer         = Buffer;

  //
  // read blocks in the same cylinder.
  // in a cylinder , there are 18 * 2 = 36 blocks
  //
  BlockCount = GetTransferBlockCount (FdcDev, LBA, NumberOfBlocks);
  while ((BlockCount != 0) && !EFI_ERROR (Status)) {
    Status = ReadWriteDataSector (FdcDev, Buffer, LBA, BlockCount, Operation);
    if (EFI_ERROR (Status)) {
      MotorOff (FdcDev);
      FddReset (FdcDev);
      return EFI_DEVICE_ERROR;
    }

    LBA += BlockCount;
    NumberOfBlocks -= BlockCount;
    Buffer      = (VOID *) ((UINTN) Buffer + BlockCount * BlockSize);
    BlockCount  = GetTransferBlockCount (FdcDev, LBA, NumberOfBlocks);
  }

  Buffer = Pointer;

  //
  // Turn the motor off
  //
  MotorOff (FdcDev);

  if (Operation == READ) {
    //
    // Cache the data read
    //
    if (LBA0 == 0 && !FdcDev->Cache) {
      FdcDev->Cache = AllocateCopyPool (BlockSize, Buffer);
    }
  }

  return EFI_SUCCESS;

}

VOID
FdcFreeCache (
  IN    FDC_BLK_IO_DEV  *FdcDev
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FdcDev  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  if (FdcDev->Cache) {
    gBS->FreePool (FdcDev->Cache);
    FdcDev->Cache = NULL;
  }
}
