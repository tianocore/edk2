/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by FVB module.

Copyright (c) 2010  - 2014, Intel Corporation. All rights reserved. <BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef _SMM_FVB_DXE_H_
#define _SMM_FVB_DXE_H_

#include <PiDxe.h>

#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Protocol/SmmCommunication.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/EventGroup.h>
#include "FvbSmmCommon.h"

#define FVB_DEVICE_SIGNATURE              SIGNATURE_32 ('F', 'V', 'B', 'S')
#define FVB_DEVICE_FROM_THIS(a)           CR (a, EFI_FVB_DEVICE, FvbInstance, FVB_DEVICE_SIGNATURE)

typedef struct {
  MEDIA_FW_VOL_DEVICE_PATH  FvDevPath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevPath;
} FV_PIWG_DEVICE_PATH;

typedef struct {
  MEMMAP_DEVICE_PATH          MemMapDevPath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevPath;
} FV_MEMMAP_DEVICE_PATH;

typedef struct {
  UINTN                                   Signature;
  EFI_DEVICE_PATH_PROTOCOL                *DevicePath;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL      FvbInstance;
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *SmmFvbInstance;
} EFI_FVB_DEVICE;

/**
  This function retrieves the attributes and current settings of the block.

  @param[in]  This       Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.

  @param[out] Attributes Pointer to EFI_FVB_ATTRIBUTES_2 in which the attributes
                         and current settings are returned. Type EFI_FVB_ATTRIBUTES_2
                         is defined in EFI_FIRMWARE_VOLUME_HEADER.

  @retval EFI_SUCCESS    The firmware volume attributes were returned.

**/
EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
     OUT   EFI_FVB_ATTRIBUTES_2                 *Attributes
  );


  /**
  Sets Volume attributes. No polarity translations are done.

  @param[in]  This        Calling context.
  @param[out] Attributes  Output buffer which contains attributes.

  @retval     EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN OUT   EFI_FVB_ATTRIBUTES_2                 *Attributes
  );


/**
  Retrieves the physical address of the device.

  @param[in]  This    A pointer to EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL.
  @param[out] Address Output buffer containing the address.

  @retval EFI_SUCCESS The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
     OUT   EFI_PHYSICAL_ADDRESS                *Address
  );


/**
  Retrieve the size of a logical block.

  @param[in]  This         Calling context.
  @param[in]  Lba          Indicates which block to return the size for.
  @param[out] BlockSize    A pointer to a caller allocated UINTN in which
                           the size of the block is returned.
  @param[out] NumOfBlocks  A pointer to a caller allocated UINTN in which the
                           number of consecutive blocks starting with Lba is
                           returned. All blocks in this range have a size of
                           BlockSize.

  @retval     EFI_SUCCESS  The function always return successfully.

**/
EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *This,
  IN       EFI_LBA                             Lba,
     OUT   UINTN                               *BlockSize,
     OUT   UINTN                               *NumOfBlocks
  );


/**
  Reads data beginning at Lba:Offset from FV. The Read terminates either
  when *NumBytes of data have been read, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This           Calling context.
  @param[in]      Lba            Block in which to begin write.
  @param[in]      Offset         Offset in the block at which to begin write
  @param[in,out]  NumBytes       On input, indicates the requested write size. On
                                 output, indicates the actual number of bytes written
  @param[in]      Buffer         Buffer containing source data for the write.

  @retval EFI_SUCCESS            The firmware volume was read successfully and
                                 contents are in Buffer
  @retval EFI_BAD_BUFFER_SIZE    Read attempted across a LBA boundary. On output,
                                 NumBytes contains the total number of bytes returned
                                 in Buffer
  @retval EFI_ACCESS_DENIED      The firmware volume is in the ReadDisabled state
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be read
  @retval EFI_INVALID_PARAMETER  NumBytes or Buffer are NULL

**/
EFI_STATUS
EFIAPI
FvbRead (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
     OUT   UINT8                                *Buffer
  );


/**
  Writes data beginning at Lba:Offset from FV. The write terminates either
  when *NumBytes of data have been written, or when a block boundary is
  reached.  *NumBytes is updated to reflect the actual number of bytes
  written. The write opertion does not include erase. This routine will
  attempt to write only the specified bytes. If the writes do not stick,
  it will return an error.

  @param[in]      This           Calling context.
  @param[in]      Lba            Block in which to begin write.
  @param[in]      Offset         Offset in the block at which to begin write.
  @param[in,out]  NumBytes       On input, indicates the requested write size. On
                                 output, indicates the actual number of bytes written
  @param[in]      Buffer         Buffer containing source data for the write.

  @retval EFI_SUCCESS            The firmware volume was written successfully
  @retval EFI_BAD_BUFFER_SIZE    Write attempted across a LBA boundary. On output,
                                 NumBytes contains the total number of bytes
                                 actually written.
  @retval EFI_ACCESS_DENIED      The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR       The block device is not functioning correctly and
                                 could not be written.
  @retval EFI_INVALID_PARAMETER  NumBytes or Buffer are NULL.

**/
EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL   *This,
  IN       EFI_LBA                              Lba,
  IN       UINTN                                Offset,
  IN OUT   UINTN                                *NumBytes,
  IN       UINT8                                *Buffer
  );


/**
  The EraseBlock() function erases one or more blocks as denoted by the
  variable argument list. The entire parameter list of blocks must be verified
  prior to erasing any blocks.  If a block is requested that does not exist
  within the associated firmware volume (it has a larger index than the last
  block of the firmware volume), the EraseBlock() function must return
  EFI_INVALID_PARAMETER without modifying the contents of the firmware volume.

  @param[in] This         Calling context.
  @param[in] ...          Starting LBA followed by Number of Lba to erase.
                          a -1 to terminate the list.

  @retval EFI_SUCCESS       The erase request was successfully completed.
  @retval EFI_ACCESS_DENIED The firmware volume is in the WriteDisabled state
  @retval EFI_DEVICE_ERROR  The block device is not functioning correctly and
                            could not be written. Firmware device may have been
                            partially erased.

**/
EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *This,
  ...
  );

#endif
