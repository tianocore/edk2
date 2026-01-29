/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpressPei.h"

/**
  Read some sectors from the device.

  @param  NamespaceInfo    The pointer to the PEI_NVME_NAMESPACE_INFO data structure.
  @param  Buffer           The buffer used to store the data read from the device.
  @param  Lba              The start block number.
  @param  Blocks           Total block number to be read.

  @retval EFI_SUCCESS            Data are read from the device.
  @retval Others                 Fail to read all the data.

**/
EFI_STATUS
ReadSectors (
  IN  PEI_NVME_NAMESPACE_INFO  *NamespaceInfo,
  OUT UINTN                    Buffer,
  IN  UINT64                   Lba,
  IN  UINT32                   Blocks
  )
{
  EFI_STATUS                                Status;
  UINT32                                    BlockSize;
  PEI_NVME_CONTROLLER_PRIVATE_DATA          *Private;
  UINT32                                    Bytes;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_PPI       *NvmePassThru;

  Private      = NamespaceInfo->Controller;
  NvmePassThru = &Private->NvmePassThruPpi;
  BlockSize    = NamespaceInfo->Media.BlockSize;
  Bytes        = Blocks * BlockSize;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;

  CommandPacket.NvmeCmd->Cdw0.Opcode = NVME_IO_READ_OPC;
  CommandPacket.NvmeCmd->Nsid        = NamespaceInfo->NamespaceId;
  CommandPacket.TransferBuffer       = (VOID *)Buffer;

  CommandPacket.TransferLength = Bytes;
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_IO_QUEUE;

  CommandPacket.NvmeCmd->Cdw10 = (UINT32)Lba;
  CommandPacket.NvmeCmd->Cdw11 = (UINT32)RShiftU64 (Lba, 32);
  CommandPacket.NvmeCmd->Cdw12 = (Blocks - 1) & 0xFFFF;

  CommandPacket.NvmeCmd->Flags = CDW10_VALID | CDW11_VALID | CDW12_VALID;

  Status = NvmePassThru->PassThru (
                           NvmePassThru,
                           NamespaceInfo->NamespaceId,
                           &CommandPacket
                           );

  return Status;
}

/**
  Read some blocks from the device.

  @param[in]  NamespaceInfo    The pointer to the PEI_NVME_NAMESPACE_INFO data structure.
  @param[out] Buffer           The Buffer used to store the Data read from the device.
  @param[in]  Lba              The start block number.
  @param[in]  Blocks           Total block number to be read.

  @retval EFI_SUCCESS          Data are read from the device.
  @retval Others               Fail to read all the data.

**/
EFI_STATUS
NvmeRead (
  IN  PEI_NVME_NAMESPACE_INFO  *NamespaceInfo,
  OUT UINTN                    Buffer,
  IN  UINT64                   Lba,
  IN  UINTN                    Blocks
  )
{
  EFI_STATUS                        Status;
  UINT32                            Retries;
  UINT32                            BlockSize;
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;
  UINT32                            MaxTransferBlocks;
  UINTN                             OrginalBlocks;

  Status        = EFI_SUCCESS;
  Retries       = 0;
  Private       = NamespaceInfo->Controller;
  BlockSize     = NamespaceInfo->Media.BlockSize;
  OrginalBlocks = Blocks;

  if (Private->ControllerData->Mdts != 0) {
    MaxTransferBlocks = (1 << (Private->ControllerData->Mdts)) * (1 << (Private->Cap.Mpsmin + 12)) / BlockSize;
  } else {
    MaxTransferBlocks = 1024;
  }

  while (Blocks > 0) {
    Status = ReadSectors (
               NamespaceInfo,
               Buffer,
               Lba,
               Blocks > MaxTransferBlocks ? MaxTransferBlocks : (UINT32)Blocks
               );
    if (EFI_ERROR (Status)) {
      Retries++;
      MaxTransferBlocks = MaxTransferBlocks >> 1;

      if ((Retries > NVME_READ_MAX_RETRY) || (MaxTransferBlocks < 1)) {
        DEBUG ((DEBUG_ERROR, "%a: ReadSectors fail, Status - %r\n", __func__, Status));
        break;
      }

      DEBUG ((
        DEBUG_BLKIO,
        "%a: ReadSectors fail, retry with smaller transfer block number - 0x%x\n",
        __func__,
        MaxTransferBlocks
        ));
      continue;
    }

    if (Blocks > MaxTransferBlocks) {
      Blocks -= MaxTransferBlocks;
      Buffer += (MaxTransferBlocks * BlockSize);
      Lba    += MaxTransferBlocks;
    } else {
      Blocks = 0;
    }
  }

  DEBUG ((
    DEBUG_BLKIO,
    "%a: Lba = 0x%08Lx, Original = 0x%08Lx, "
    "Remaining = 0x%08Lx, BlockSize = 0x%x, Status = %r\n",
    __func__,
    Lba,
    (UINT64)OrginalBlocks,
    (UINT64)Blocks,
    BlockSize,
    Status
    ));
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
NvmeBlockIoPeimGetDeviceNo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;

  if ((This == NULL) || (NumberBlockDevices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private             = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);
  *NumberBlockDevices = Private->ActiveNamespaceNum;

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
NvmeBlockIoPeimGetMediaInfo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;

  if ((This == NULL) || (MediaInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);

  if ((DeviceIndex == 0) || (DeviceIndex > Private->ActiveNamespaceNum)) {
    return EFI_INVALID_PARAMETER;
  }

  MediaInfo->DeviceType   = (EFI_PEI_BLOCK_DEVICE_TYPE)EDKII_PEI_BLOCK_DEVICE_TYPE_NVME;
  MediaInfo->MediaPresent = TRUE;
  MediaInfo->LastBlock    = (UINTN)Private->NamespaceInfo[DeviceIndex-1].Media.LastBlock;
  MediaInfo->BlockSize    = Private->NamespaceInfo[DeviceIndex-1].Media.BlockSize;

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
NvmeBlockIoPeimReadBlocks (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;
  PEI_NVME_NAMESPACE_INFO           *NamespaceInfo;
  UINT32                            BlockSize;
  UINTN                             NumberOfBlocks;

  Private = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO (This);

  //
  // Check parameters
  //
  if ((This == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if ((DeviceIndex == 0) || (DeviceIndex > Private->ActiveNamespaceNum)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check BufferSize and StartLBA
  //
  NamespaceInfo = &(Private->NamespaceInfo[DeviceIndex - 1]);
  BlockSize     = NamespaceInfo->Media.BlockSize;
  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (StartLBA > NamespaceInfo->Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if (NumberOfBlocks - 1 > NamespaceInfo->Media.LastBlock - StartLBA) {
    return EFI_INVALID_PARAMETER;
  }

  return NvmeRead (NamespaceInfo, (UINTN)Buffer, StartLBA, NumberOfBlocks);
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
NvmeBlockIoPeimGetDeviceNo2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  OUT UINTN                           *NumberBlockDevices
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;

  if ((This == NULL) || (NumberBlockDevices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private             = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);
  *NumberBlockDevices = Private->ActiveNamespaceNum;

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
NvmeBlockIoPeimGetMediaInfo2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  OUT EFI_PEI_BLOCK_IO2_MEDIA         *MediaInfo
  )
{
  EFI_STATUS                        Status;
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;
  EFI_PEI_BLOCK_IO_MEDIA            Media;

  if ((This == NULL) || (MediaInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);

  Status = NvmeBlockIoPeimGetMediaInfo (
             PeiServices,
             &Private->BlkIoPpi,
             DeviceIndex,
             &Media
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (
    MediaInfo,
    &(Private->NamespaceInfo[DeviceIndex - 1].Media),
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
NvmeBlockIoPeimReadBlocks2 (
  IN  EFI_PEI_SERVICES                **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI  *This,
  IN  UINTN                           DeviceIndex,
  IN  EFI_PEI_LBA                     StartLBA,
  IN  UINTN                           BufferSize,
  OUT VOID                            *Buffer
  )
{
  PEI_NVME_CONTROLLER_PRIVATE_DATA  *Private;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2 (This);
  return NvmeBlockIoPeimReadBlocks (
           PeiServices,
           &Private->BlkIoPpi,
           DeviceIndex,
           StartLBA,
           BufferSize,
           Buffer
           );
}
