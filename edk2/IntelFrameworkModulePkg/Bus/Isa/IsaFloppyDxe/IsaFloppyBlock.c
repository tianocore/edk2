/**@file
  ISA Floppy Driver
  1. Support two types diskette drive  
     1.44M drive and 2.88M drive (and now only support 1.44M)
  2. Support two diskette drives
  3. Use DMA channel 2 to transfer data
  4. Do not use interrupt
  5. Support diskette change line signal and write protect
  
  Implement the Block IO interface
  
Copyright (c) 2006 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "IsaFloppy.h"

/**
  Reset the Floppy Logic Drive, call the FddReset function   
  
  @param This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param ExtendedVerification BOOLEAN: Indicate that the driver may perform a more 
                    exhaustive verification operation of the device during 
                    reset, now this par is ignored in this driver          
  @retval  EFI_SUCCESS:      The Floppy Logic Drive is reset
  @retval  EFI_DEVICE_ERROR: The Floppy Logic Drive is not functioning correctly 
                      and can not be reset

**/
EFI_STATUS
EFIAPI
FdcReset (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                ExtendedVerification
  )
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

/**
  Flush block via fdd controller
  
  @param  This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FddFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  //
  // Not supported yet
  //
  return EFI_SUCCESS;
}

/**
  Common report status code interface
  
  @param This  Pointer of FDC_BLK_IO_DEV instance
  @param Read  Error type: read or write?
**/
VOID
FddReportStatus (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                Read
  )
{
  FDC_BLK_IO_DEV  *FdcDev;

  FdcDev = FDD_BLK_IO_FROM_THIS (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_ERROR_CODE,
    ((Read) ? EFI_P_EC_INPUT_ERROR : EFI_P_EC_OUTPUT_ERROR) | EFI_PERIPHERAL_REMOVABLE_MEDIA,
    FdcDev->DevicePath
    );
}

/**
  Read the requested number of blocks from the device   
  
  @param This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param MediaId UINT32:    The media id that the read request is for    
  @param  LBA EFI_LBA:     The starting logic block address to read from on the device
  @param  BufferSize UINTN:  The size of the Buffer in bytes
  @param  Buffer VOID *:     A pointer to the destination buffer for the data
  
  @retval  EFI_SUCCESS:     The data was read correctly from the device
  @retval  EFI_DEVICE_ERROR:The device reported an error while attempting to perform
                     the read operation
  @retval  EFI_NO_MEDIA:    There is no media in the device
  @retval  EFI_MEDIA_CHANGED:   The MediaId is not for the current media
  @retval  EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
  @retval  EFI_INVALID_PARAMETER:The read request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 

**/
EFI_STATUS
EFIAPI
FddReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                LBA,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, LBA, BufferSize, READ, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, TRUE);
  }

  return Status;
}

/**
  Write a specified number of blocks to the device   
  
  @param  This EFI_BLOCK_IO *: A pointer to the Block I/O protocol interface
  @param  MediaId UINT32:    The media id that the write request is for   
  @param  LBA EFI_LBA:     The starting logic block address to be written
  @param  BufferSize UINTN:  The size in bytes in Buffer
  @param  Buffer VOID *:     A pointer to the source buffer for the data
  
  @retval  EFI_SUCCESS:     The data were written correctly to the device
  @retval  EFI_WRITE_PROTECTED: The device can not be written to 
  @retval  EFI_NO_MEDIA:    There is no media in the device
  @retval  EFI_MEDIA_CHANGED:   The MediaId is not for the current media
  @retval  EFI_DEVICE_ERROR:  The device reported an error while attempting to perform 
                       the write operation 
  @retval  EFI_BAD_BUFFER_SIZE: The BufferSize parameter is not a multiple of the 
                         intrinsic block size of the device
  @retval  EFI_INVALID_PARAMETER:The write request contains LBAs that are not valid, 
                          or the buffer is not on proper alignment 
**/
EFI_STATUS
EFIAPI
FddWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                LBA,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, LBA, BufferSize, WRITE, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, FALSE);
  }

  return Status;
}

/**
  Read or Write a number of blocks to floppy device

  @param This     Pointer to instance of EFI_BLOCK_IO_PROTOCOL
  @param MediaId  The media id of read/write request
  @param LBA      The starting logic block address to read from on the device
  @param BufferSize The size of the Buffer in bytes
  @param Operation   - GC_TODO: add argument description
  @param Buffer      - GC_TODO: add argument description

  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_NO_MEDIA - GC_TODO: Add description for return value
  @retval EFI_MEDIA_CHANGED - GC_TODO: Add description for return value
  @retval EFI_WRITE_PROTECTED - GC_TODO: Add description for return value
  @retval EFI_BAD_BUFFER_SIZE - GC_TODO: Add description for return value
  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
FddReadWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                LBA,
  IN  UINTN                  BufferSize,
  IN  BOOLEAN                Operation,
  OUT VOID                   *Buffer
  )
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

/**
  Common interface for free cache 
  
  @param FdcDev  Pointer of FDC_BLK_IO_DEV instance
  
**/
VOID
FdcFreeCache (
  IN    FDC_BLK_IO_DEV  *FdcDev
  )
{
  if (FdcDev->Cache) {
    gBS->FreePool (FdcDev->Cache);
    FdcDev->Cache = NULL;
  }
}
