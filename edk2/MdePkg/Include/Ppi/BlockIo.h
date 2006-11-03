/** @file
  This file declares BlockIo PPI used to access block-oriented storage devices

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  BlockIo.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI Recovery Spec
  Version 0.9

**/

#ifndef _PEI_BLOCK_IO_H_
#define _PEI_BLOCK_IO_H_

#define EFI_PEI_VIRTUAL_BLOCK_IO_PPI \
  { \
    0x695d8aa1, 0x42ee, 0x4c46, {0x80, 0x5c, 0x6e, 0xa6, 0xbc, 0xe7, 0x99, 0xe3 } \
  }

typedef struct _EFI_PEI_RECOVERY_BLOCK_IO_PPI EFI_PEI_RECOVERY_BLOCK_IO_PPI;

typedef UINT64  EFI_PEI_LBA;

typedef enum {
  LegacyFloppy  = 0,
  IdeCDROM      = 1,
  IdeLS120      = 2,
  UsbMassStorage= 3,
  MaxDeviceType
} EFI_PEI_BLOCK_DEVICE_TYPE;

typedef struct {
  EFI_PEI_BLOCK_DEVICE_TYPE  DeviceType;
  BOOLEAN                    MediaPresent;
  UINTN                      LastBlock;
  UINTN                      BlockSize;
} EFI_PEI_BLOCK_IO_MEDIA;

/**
  Gets the count of block I/O devices that one specific block driver detects.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  This               Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param  NumberBlockDevices The number of block I/O devices discovered.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_NUMBER_BLOCK_DEVICES) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI            *This,
  OUT UINTN                                    *NumberBlockDevices
  );

/**
  Gets a block device's media information.

  @param  PeiServices    General-purpose services that are available to every PEIM
  @param  This           Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param  DeviceIndex    Specifies the block device to which the function
                         wants to talk. Because the driver that implements Block I/O PPIs
                         will manage multiple block devices, the PPIs that want to talk to a single
                         device must specify the device index that was assigned during the enumeration
                         process. This index is a number from one to NumberBlockDevices.
  @param  MediaInfo      The media information of the specified block media.

  @retval EFI_SUCCESS           Media information about the specified block device was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_DEVICE_MEDIA_INFORMATION) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI            *This,
  IN  UINTN                                    DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA                   *MediaInfo
  );

/**
  Reads the requested number of blocks from the specified block device.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  This           Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param  DeviceIndex    Specifies the block device to which the function wants to talk.
  @param  StartLBA       The starting logical block address (LBA) to read from on the device
  @param  BufferSize     The size of the Buffer in bytes. This number must
                         be a multiple of the intrinsic block size of the device.
  @param  Buffer         A pointer to the destination buffer for the data.
                         The caller is responsible for the ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while attempting to perform the read operation.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not properly aligned.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of
                                the intrinsic block size of the device.
  @retval EFI_NO_MEDIA          There is no media in the device.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_READ_BLOCKS) (
  IN  EFI_PEI_SERVICES                         **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI            *This,
  IN  UINTN                                    DeviceIndex,
  IN  EFI_PEI_LBA                              StartLBA,
  IN  UINTN                                    BufferSize,
  OUT VOID                                     *Buffer
  );

/**
  @par Ppi Description:
  EFI_PEI_RECOVERY_BLOCK_IO_PPI provides the services that are required 
  to access a block I/O device during PEI recovery boot mode. 

  @param GetNumberOfBlockDevices
  Gets the number of block I/O devices that the specific block driver manages.

  @param GetBlockDeviceMediaInfo
  Gets the specified media information.

  @param ReadBlocks
  Reads the requested number of blocks from the specified block device.

**/
struct _EFI_PEI_RECOVERY_BLOCK_IO_PPI {
  EFI_PEI_GET_NUMBER_BLOCK_DEVICES      GetNumberOfBlockDevices;
  EFI_PEI_GET_DEVICE_MEDIA_INFORMATION  GetBlockDeviceMediaInfo;
  EFI_PEI_READ_BLOCKS                   ReadBlocks;
};

extern EFI_GUID gEfiPeiBlockIoPpiGuid;

#endif
