/** @file 
  Implementation of the EFI Block IO Protocol for ISA Floppy driver
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IsaFloppy.h"

/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.
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
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

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
  Common report status code interface.
  
  @param This  Pointer of FDC_BLK_IO_DEV instance
  @param Read  Read or write operation when error occurrs
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
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
FddReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, Lba, BufferSize, READ, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, TRUE);
  }

  return Status;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
FddWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS  Status;

  Status = FddReadWriteBlocks (This, MediaId, Lba, BufferSize, WRITE, Buffer);

  if (EFI_ERROR (Status)) {
    FddReportStatus (This, FALSE);
  }

  return Status;
}

/**
  Read or Write a number of blocks to floppy disk

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Operation  Specifies the read or write operation.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid, 
                                or the buffer is not on proper alignment.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.

**/
EFI_STATUS
FddReadWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
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
  EFI_LBA             Lba0;
  UINT8               *Pointer;

  //
  // Get the intrinsic block size
  //
  Media     = This->Media;
  BlockSize = Media->BlockSize;
  FdcDev    = FDD_BLK_IO_FROM_THIS (This);

  if (Operation == WRITE) {
    if (Lba == 0) {
      FdcFreeCache (FdcDev);
    }
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

  if (BufferSize == 0) {
    MotorOff (FdcDev);
    return EFI_SUCCESS;
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
  if (Buffer == NULL) {
    MotorOff (FdcDev);
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize % BlockSize != 0) {
    MotorOff (FdcDev);
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba > Media->LastBlock) {
    MotorOff (FdcDev);
    return EFI_INVALID_PARAMETER;
  }

  if (((BufferSize / BlockSize) + Lba - 1) > Media->LastBlock) {
    MotorOff (FdcDev);
    return EFI_INVALID_PARAMETER;
  }

  if (Operation == READ) {
    //
    // See if the data that is being read is already in the cache
    //
    if (FdcDev->Cache != NULL) {
      if (Lba == 0 && BufferSize == BlockSize) {
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
  Lba0            = Lba;
  Pointer         = Buffer;

  //
  // read blocks in the same cylinder.
  // in a cylinder , there are 18 * 2 = 36 blocks
  //
  BlockCount = GetTransferBlockCount (FdcDev, Lba, NumberOfBlocks);
  while ((BlockCount != 0) && !EFI_ERROR (Status)) {
    Status = ReadWriteDataSector (FdcDev, Buffer, Lba, BlockCount, Operation);
    if (EFI_ERROR (Status)) {
      MotorOff (FdcDev);
      FddReset (FdcDev);
      return EFI_DEVICE_ERROR;
    }

    Lba += BlockCount;
    NumberOfBlocks -= BlockCount;
    Buffer      = (VOID *) ((UINTN) Buffer + BlockCount * BlockSize);
    BlockCount  = GetTransferBlockCount (FdcDev, Lba, NumberOfBlocks);
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
    if (Lba0 == 0 && FdcDev->Cache == NULL) {
      FdcDev->Cache = AllocateCopyPool (BlockSize, Buffer);
    }
  }

  return EFI_SUCCESS;

}

/**
  Free cache for a floppy disk.
  
  @param FdcDev  A Pointer to FDC_BLK_IO_DEV instance
  
**/
VOID
FdcFreeCache (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  if (FdcDev->Cache != NULL) {
    FreePool (FdcDev->Cache);
    FdcDev->Cache = NULL;
  }
}
