/** @file
  The AhciPei driver is used to manage ATA hard disk device working under AHCI
  mode at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AhciPei.h"

/**
  Traverse the attached ATA devices list to find out the device with given index.

  @param[in] Private        A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA
                            instance.
  @param[in] DeviceIndex    The device index.

  @retval    The pointer to the PEI_AHCI_ATA_DEVICE_DATA structure of the device
             info to access.

**/
PEI_AHCI_ATA_DEVICE_DATA *
SearchDeviceByIndex (
  IN PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINTN                             DeviceIndex
  )
{
  PEI_AHCI_ATA_DEVICE_DATA  *DeviceData;
  LIST_ENTRY                *Node;

  if ((DeviceIndex == 0) || (DeviceIndex > Private->ActiveDevices)) {
    return NULL;
  }

  Node = GetFirstNode (&Private->DeviceList);
  while (!IsNull (&Private->DeviceList, Node)) {
    DeviceData = AHCI_PEI_ATA_DEVICE_INFO_FROM_THIS (Node);

    if (DeviceData->DeviceIndex == DeviceIndex) {
      return DeviceData;
    }

    Node = GetNextNode (&Private->DeviceList, Node);
  }

  return NULL;
}

/**
  Read a number of blocks from ATA device.

  This function performs ATA pass through transactions to read data from ATA device.
  It may separate the read request into several ATA pass through transactions.

  @param[in]     DeviceData        The pointer to the PEI_AHCI_ATA_DEVICE_DATA
                                   data structure.
  @param[in,out] Buffer            The pointer to the current transaction buffer.
  @param[in]     StartLba          The starting logical block address to be accessed.
  @param[in]     NumberOfBlocks    The block number or sector count of the transfer.

  @retval EFI_SUCCESS    The data transfer is complete successfully.
  @return Others         Some error occurs when transferring data.

**/
EFI_STATUS
AccessAtaDevice (
  IN     PEI_AHCI_ATA_DEVICE_DATA  *DeviceData,
  IN OUT UINT8                     *Buffer,
  IN     EFI_LBA                   StartLba,
  IN     UINTN                     NumberOfBlocks
  )
{
  EFI_STATUS  Status;
  UINTN       MaxTransferBlockNumber;
  UINTN       TransferBlockNumber;
  UINTN       BlockSize;

  //
  // Ensure Lba48Bit is a valid boolean value
  //
  ASSERT ((UINTN)DeviceData->Lba48Bit < 2);
  if ((UINTN)DeviceData->Lba48Bit >= 2) {
    return EFI_INVALID_PARAMETER;
  }

  Status                 = EFI_SUCCESS;
  MaxTransferBlockNumber = mMaxTransferBlockNumber[DeviceData->Lba48Bit];
  BlockSize              = DeviceData->Media.BlockSize;

  do {
    if (NumberOfBlocks > MaxTransferBlockNumber) {
      TransferBlockNumber = MaxTransferBlockNumber;
      NumberOfBlocks     -= MaxTransferBlockNumber;
    } else {
      TransferBlockNumber = NumberOfBlocks;
      NumberOfBlocks      = 0;
    }

    DEBUG ((
      DEBUG_BLKIO,
      "%a: Blocking AccessAtaDevice, TransferBlockNumber = %x; StartLba = %x\n",
      __func__,
      TransferBlockNumber,
      StartLba
      ));

    Status = TransferAtaDevice (
               DeviceData,
               Buffer,
               StartLba,
               (UINT32)TransferBlockNumber,
               FALSE  // Read
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    StartLba += TransferBlockNumber;
    Buffer   += TransferBlockNumber * BlockSize;
  } while (NumberOfBlocks > 0);

  return Status;
}

/**
  Read specified bytes from Lba from the device.

  @param[in]  DeviceData    The pointer to the PEI_AHCI_ATA_DEVICE_DATA data structure.
  @param[out] Buffer        The Buffer used to store the Data read from the device.
  @param[in]  StartLba      The start block number.
  @param[in]  BufferSize    Total bytes to be read.

  @retval EFI_SUCCESS    Data are read from the device.
  @retval Others         Fail to read all the data.

**/
EFI_STATUS
AhciRead (
  IN  PEI_AHCI_ATA_DEVICE_DATA  *DeviceData,
  OUT VOID                      *Buffer,
  IN  EFI_LBA                   StartLba,
  IN  UINTN                     BufferSize
  )
{
  EFI_STATUS  Status;
  UINTN       BlockSize;
  UINTN       NumberOfBlocks;

  //
  // Check parameters.
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BlockSize = DeviceData->Media.BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (StartLba > DeviceData->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if (NumberOfBlocks - 1 > DeviceData->Media.LastBlock - StartLba) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Invoke low level AtaDevice Access Routine.
  //
  Status = AccessAtaDevice (DeviceData, Buffer, StartLba, NumberOfBlocks);

  return Status;
}

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects. If no device is detected, then the function
  will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
EFIAPI
AhciBlockIoGetDeviceNo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;

  if ((This == NULL) || (NumberBlockDevices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private             = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);
  *NumberBlockDevices = Private->ActiveDevices;

  return EFI_SUCCESS;
}

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @par Note:
      The MediaInfo structure describes an enumeration of possible block device
      types.  This enumeration exists because no device paths are actually passed
      across interfaces that describe the type or class of hardware that is publishing
      the block I/O interface. This enumeration will allow for policy decisions
      in the Recovery PEIM, such as "Try to recover from legacy floppy first,
      LS-120 second, CD-ROM third." If there are multiple partitions abstracted
      by a given device type, they should be reported in ascending order; this
      order also applies to nested partitions, such as legacy MBR, where the
      outermost partitions would have precedence in the reporting order. The
      same logic applies to systems such as IDE that have precedence relationships
      like "Master/Slave" or "Primary/Secondary". The master device should be
      reported first, the slave second.

  @retval EFI_SUCCESS        Media information about the specified block device
                             was obtained successfully.
  @retval EFI_DEVICE_ERROR   Cannot get the media information due to a hardware
                             error.

**/
EFI_STATUS
EFIAPI
AhciBlockIoGetMediaInfo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;
  PEI_AHCI_ATA_DEVICE_DATA          *DeviceData;

  if ((This == NULL) || (MediaInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private    = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);
  DeviceData = SearchDeviceByIndex (Private, DeviceIndex);
  if (DeviceData == NULL) {
    return EFI_NOT_FOUND;
  }

  MediaInfo->DeviceType   = (EFI_PEI_BLOCK_DEVICE_TYPE)EDKII_PEI_BLOCK_DEVICE_TYPE_ATA_HARD_DISK;
  MediaInfo->MediaPresent = TRUE;
  MediaInfo->LastBlock    = (UINTN)DeviceData->Media.LastBlock;
  MediaInfo->BlockSize    = DeviceData->Media.BlockSize;

  return EFI_SUCCESS;
}

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
AhciBlockIoReadBlocks (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;
  PEI_AHCI_ATA_DEVICE_DATA          *DeviceData;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private    = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);
  DeviceData = SearchDeviceByIndex (Private, DeviceIndex);
  if (DeviceData == NULL) {
    return EFI_NOT_FOUND;
  }

  return AhciRead (DeviceData, Buffer, StartLBA, BufferSize);
}

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects. If no device is detected, then the function
  will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
EFIAPI
AhciBlockIoGetDeviceNo2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  OUT UINTN                           *NumberBlockDevices
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;

  if ((This == NULL) || (NumberBlockDevices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private             = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);
  *NumberBlockDevices = Private->ActiveDevices;

  return EFI_SUCCESS;
}

/**
  Gets a block device's media information.

  This function will provide the caller with the specified block device's media
  information. If the media changes, calling this function will update the media
  information accordingly.

  @param[in]  PeiServices   General-purpose services that are available to every
                            PEIM
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, the PPIs that
                            want to talk to a single device must specify the
                            device index that was assigned during the enumeration
                            process. This index is a number from one to
                            NumberBlockDevices.
  @param[out] MediaInfo     The media information of the specified block media.
                            The caller is responsible for the ownership of this
                            data structure.

  @par Note:
      The MediaInfo structure describes an enumeration of possible block device
      types.  This enumeration exists because no device paths are actually passed
      across interfaces that describe the type or class of hardware that is publishing
      the block I/O interface. This enumeration will allow for policy decisions
      in the Recovery PEIM, such as "Try to recover from legacy floppy first,
      LS-120 second, CD-ROM third." If there are multiple partitions abstracted
      by a given device type, they should be reported in ascending order; this
      order also applies to nested partitions, such as legacy MBR, where the
      outermost partitions would have precedence in the reporting order. The
      same logic applies to systems such as IDE that have precedence relationships
      like "Master/Slave" or "Primary/Secondary". The master device should be
      reported first, the slave second.

  @retval EFI_SUCCESS        Media information about the specified block device
                             was obtained successfully.
  @retval EFI_DEVICE_ERROR   Cannot get the media information due to a hardware
                             error.

**/
EFI_STATUS
EFIAPI
AhciBlockIoGetMediaInfo2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  OUT EFI_PEI_BLOCK_IO2_MEDIA         *MediaInfo
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;
  PEI_AHCI_ATA_DEVICE_DATA          *DeviceData;

  if ((This == NULL) || (MediaInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private    = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);
  DeviceData = SearchDeviceByIndex (Private, DeviceIndex);
  if (DeviceData == NULL) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    MediaInfo,
    &DeviceData->Media,
    sizeof (EFI_PEI_BLOCK_IO2_MEDIA)
    );

  return EFI_SUCCESS;
}

/**
  Reads the requested number of blocks from the specified block device.

  The function reads the requested number of blocks from the device. All the
  blocks are read, or an error is returned. If there is no media in the device,
  the function returns EFI_NO_MEDIA.

  @param[in]  PeiServices   General-purpose services that are available to
                            every PEIM.
  @param[in]  This          Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI instance.
  @param[in]  DeviceIndex   Specifies the block device to which the function wants
                            to talk. Because the driver that implements Block I/O
                            PPIs will manage multiple block devices, PPIs that
                            want to talk to a single device must specify the device
                            index that was assigned during the enumeration process.
                            This index is a number from one to NumberBlockDevices.
  @param[in]  StartLBA      The starting logical block address (LBA) to read from
                            on the device
  @param[in]  BufferSize    The size of the Buffer in bytes. This number must be
                            a multiple of the intrinsic block size of the device.
  @param[out] Buffer        A pointer to the destination buffer for the data.
                            The caller is responsible for the ownership of the
                            buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not properly aligned.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.

**/
EFI_STATUS
EFIAPI
AhciBlockIoReadBlocks2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  IN  EFI_PEI_LBA                     StartLBA,
  IN  UINTN                           BufferSize,
  OUT VOID                            *Buffer
  )
{
  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GET_AHCI_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);
  return AhciBlockIoReadBlocks (
           PeiServices,
           &Private->BlkIoPpi,
           DeviceIndex,
           StartLBA,
           BufferSize,
           Buffer
           );
}
