/** @file

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EmmcBlockIoPei.h"

//
// Template for EMMC HC Slot Data.
//
EMMC_PEIM_HC_SLOT   gEmmcHcSlotTemplate = {
  EMMC_PEIM_SLOT_SIG,             // Signature
  {                               // Media
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    },
    {
      MSG_EMMC_DP,
      FALSE,
      TRUE,
      FALSE,
      0x200,
      0
    }
  },
  0,                              // MediaNum
  {                               // PartitionType
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown,
    EmmcPartitionUnknown
  },
  0,                              // EmmcHcBase
  {                               // Capability
    0,
  },
  {                               // Csd
    0,
  },
  {                               // ExtCsd
    {0},
  },
  TRUE,                           // SectorAddressing
  NULL                            // Private
};

//
// Template for EMMC HC Private Data.
//
EMMC_PEIM_HC_PRIVATE_DATA gEmmcHcPrivateTemplate = {
  EMMC_PEIM_SIG,                  // Signature
  NULL,                           // Pool
  {                               // BlkIoPpi
    EmmcBlockIoPeimGetDeviceNo,
    EmmcBlockIoPeimGetMediaInfo,
    EmmcBlockIoPeimReadBlocks
  },
  {                               // BlkIo2Ppi
    EFI_PEI_RECOVERY_BLOCK_IO2_PPI_REVISION,
    EmmcBlockIoPeimGetDeviceNo2,
    EmmcBlockIoPeimGetMediaInfo2,
    EmmcBlockIoPeimReadBlocks2
  },
  {                               // BlkIoPpiList
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiPeiVirtualBlockIoPpiGuid,
    NULL
  },
  {                               // BlkIo2PpiList
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiVirtualBlockIo2PpiGuid,
    NULL
  },
  {                               // EndOfPeiNotifyList
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    EmmcBlockIoPeimEndOfPei
  },
  {                               // Slot
    {
      0,
    },
    {
      0,
    },
    {
      0,
    },
    {
      0,
    },
    {
      0,
    },
    {
      0,
    }
  },
  0,                              // SlotNum
  0                               // TotalBlkIoDevices
};
/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
EFIAPI
EmmcBlockIoPeimGetDeviceNo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  EMMC_PEIM_HC_PRIVATE_DATA   *Private;

  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);
  *NumberBlockDevices = Private->TotalBlkIoDevices;
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
EmmcBlockIoPeimGetMediaInfo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  )
{
  EMMC_PEIM_HC_PRIVATE_DATA          *Private;
  UINT8                              SlotNum;
  UINT8                              MediaNum;
  UINT8                              Location;
  BOOLEAN                            Found;

  Found   = FALSE;
  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);

  if ((DeviceIndex == 0) || (DeviceIndex > Private->TotalBlkIoDevices)) {
    return EFI_INVALID_PARAMETER;
  }

  Location = 0;
  MediaNum = 0;
  for (SlotNum = 0; SlotNum < Private->SlotNum; SlotNum++) {
    for (MediaNum = 0; MediaNum < Private->Slot[SlotNum].MediaNum; MediaNum++) {
      Location ++;
      if (Location == DeviceIndex) {
        Found = TRUE;
        break;
      }
    }
    if (Found) {
      break;
    }
  }

  MediaInfo->DeviceType   = EMMC;
  MediaInfo->MediaPresent = TRUE;
  MediaInfo->LastBlock    = (UINTN)Private->Slot[SlotNum].Media[MediaNum].LastBlock;
  MediaInfo->BlockSize    = Private->Slot[SlotNum].Media[MediaNum].BlockSize;

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
EmmcBlockIoPeimReadBlocks (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  )
{
  EFI_STATUS                         Status;
  UINT32                             BlockSize;
  UINTN                              NumberOfBlocks;
  EMMC_PEIM_HC_PRIVATE_DATA          *Private;
  UINT8                              SlotNum;
  UINT8                              MediaNum;
  UINT8                              Location;
  UINT8                              PartitionConfig;
  UINTN                              Remaining;
  UINT32                             MaxBlock;
  BOOLEAN                            Found;

  Status  = EFI_SUCCESS;
  Found   = FALSE;
  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);

  //
  // Check parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if ((DeviceIndex == 0) || (DeviceIndex > Private->TotalBlkIoDevices)) {
    return EFI_INVALID_PARAMETER;
  }

  Location = 0;
  MediaNum = 0;
  for (SlotNum = 0; SlotNum < Private->SlotNum; SlotNum++) {
    for (MediaNum = 0; MediaNum < Private->Slot[SlotNum].MediaNum; MediaNum++) {
      Location ++;
      if (Location == DeviceIndex) {
        Found = TRUE;
        break;
      }
    }
    if (Found) {
      break;
    }
  }

  BlockSize = Private->Slot[SlotNum].Media[MediaNum].BlockSize;
  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (StartLBA > Private->Slot[SlotNum].Media[MediaNum].LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  //
  // Check if needs to switch partition access.
  //
  PartitionConfig = Private->Slot[SlotNum].ExtCsd.PartitionConfig;
  if ((PartitionConfig & 0x7) != Private->Slot[SlotNum].PartitionType[MediaNum]) {
    PartitionConfig &= (UINT8)~0x7;
    PartitionConfig |= Private->Slot[SlotNum].PartitionType[MediaNum];
    Status = EmmcPeimSwitch (
               &Private->Slot[SlotNum],
               0x3,
               OFFSET_OF (EMMC_EXT_CSD, PartitionConfig),
               PartitionConfig,
               0x0
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Private->Slot[SlotNum].ExtCsd.PartitionConfig = PartitionConfig;
  }
  //
  // Start to execute data transfer. The max block number in single cmd is 65535 blocks.
  //
  Remaining = NumberOfBlocks;
  MaxBlock  = 0xFFFF;

  while (Remaining > 0) {
    if (Remaining <= MaxBlock) {
      NumberOfBlocks = Remaining;
    } else {
      NumberOfBlocks = MaxBlock;
    }

    Status = EmmcPeimSetBlkCount (&Private->Slot[SlotNum], (UINT16)NumberOfBlocks);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferSize = NumberOfBlocks * BlockSize;
    Status = EmmcPeimRwMultiBlocks (&Private->Slot[SlotNum], StartLBA, BlockSize, Buffer, BufferSize, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    StartLBA  += NumberOfBlocks;
    Buffer     = (UINT8*)Buffer + BufferSize;
    Remaining -= NumberOfBlocks;
  }
  return Status;
}

/**
  Gets the count of block I/O devices that one specific block driver detects.

  This function is used for getting the count of block I/O devices that one
  specific block driver detects.  To the PEI ATAPI driver, it returns the number
  of all the detected ATAPI devices it detects during the enumeration process.
  To the PEI legacy floppy driver, it returns the number of all the legacy
  devices it finds during its enumeration process. If no device is detected,
  then the function will return zero.

  @param[in]  PeiServices          General-purpose services that are available
                                   to every PEIM.
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO2_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          The operation performed successfully.

**/
EFI_STATUS
EFIAPI
EmmcBlockIoPeimGetDeviceNo2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  EMMC_PEIM_HC_PRIVATE_DATA   *Private;

  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);
  *NumberBlockDevices = Private->TotalBlkIoDevices;

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
EmmcBlockIoPeimGetMediaInfo2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO2_MEDIA        *MediaInfo
  )
{
  EFI_STATUS                         Status;
  EMMC_PEIM_HC_PRIVATE_DATA          *Private;
  EFI_PEI_BLOCK_IO_MEDIA             Media;
  UINT8                              SlotNum;
  UINT8                              MediaNum;
  UINT8                              Location;
  BOOLEAN                            Found;

  Found   = FALSE;
  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);

  Status  = EmmcBlockIoPeimGetMediaInfo (
              PeiServices,
              &Private->BlkIoPpi,
              DeviceIndex,
              &Media
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Location = 0;
  MediaNum = 0;
  for (SlotNum = 0; SlotNum < Private->SlotNum; SlotNum++) {
    for (MediaNum = 0; MediaNum < Private->Slot[SlotNum].MediaNum; MediaNum++) {
      Location ++;
      if (Location == DeviceIndex) {
        Found = TRUE;
        break;
      }
    }
    if (Found) {
      break;
    }
  }

  CopyMem (MediaInfo, &(Private->Slot[SlotNum].Media[MediaNum]), sizeof (EFI_PEI_BLOCK_IO2_MEDIA));
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
EmmcBlockIoPeimReadBlocks2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  )
{
  EFI_STATUS                         Status;
  EMMC_PEIM_HC_PRIVATE_DATA          *Private;

  Status    = EFI_SUCCESS;
  Private   = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);

  Status  = EmmcBlockIoPeimReadBlocks (
              PeiServices,
              &Private->BlkIoPpi,
              DeviceIndex,
              StartLBA,
              BufferSize,
              Buffer
              );
  return Status;
}

/**
  One notified function to cleanup the allocated DMA buffers at the end of PEI.

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully

**/
EFI_STATUS
EFIAPI
EmmcBlockIoPeimEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EMMC_PEIM_HC_PRIVATE_DATA       *Private;

  Private = GET_EMMC_PEIM_HC_PRIVATE_DATA_FROM_THIS_NOTIFY (NotifyDescriptor);

  if ((Private->Pool != NULL) && (Private->Pool->Head != NULL)) {
    EmmcPeimFreeMemPool (Private->Pool);
  }

  return EFI_SUCCESS;
}

/**
  The user code starts with this function.

  @param  FileHandle             Handle of the file being invoked.
  @param  PeiServices            Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            The driver is successfully initialized.
  @retval Others                 Can't initialize the driver.

**/
EFI_STATUS
EFIAPI
InitializeEmmcBlockIoPeim (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                       Status;
  EMMC_PEIM_HC_PRIVATE_DATA        *Private;
  EDKII_SD_MMC_HOST_CONTROLLER_PPI *SdMmcHcPpi;
  UINT32                           Index;
  UINT32                           PartitionIndex;
  UINTN                            *MmioBase;
  UINT8                            BarNum;
  UINT8                            SlotNum;
  UINT8                            MediaNum;
  UINT8                            Controller;
  UINT64                           Capacity;
  EMMC_EXT_CSD                     *ExtCsd;
  EMMC_HC_SLOT_CAP                 Capability;
  EMMC_PEIM_HC_SLOT                *Slot;
  UINT32                           SecCount;
  UINT32                           GpSizeMult;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  //
  // locate Emmc host controller PPI
  //
  Status = PeiServicesLocatePpi (
             &gEdkiiPeiSdMmcHostControllerPpiGuid,
             0,
             NULL,
             (VOID **) &SdMmcHcPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  IoMmuInit ();

  Controller = 0;
  MmioBase   = NULL;
  while (TRUE) {
    Status = SdMmcHcPpi->GetSdMmcHcMmioBar (SdMmcHcPpi, Controller, &MmioBase, &BarNum);
    //
    // When status is error, meant no controller is found
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    if (BarNum == 0) {
      Controller++;
      continue;
    }

    Private = AllocateCopyPool (sizeof (EMMC_PEIM_HC_PRIVATE_DATA), &gEmmcHcPrivateTemplate);
    if (Private == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
    Private->BlkIoPpiList.Ppi  = (VOID*)&Private->BlkIoPpi;
    Private->BlkIo2PpiList.Ppi = (VOID*)&Private->BlkIo2Ppi;
    //
    // Initialize the memory pool which will be used in all transactions.
    //
    Status = EmmcPeimInitMemPool (Private);
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    for (Index = 0; Index < BarNum; Index++) {
      Status = EmmcPeimHcGetCapability (MmioBase[Index], &Capability);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (Capability.SlotType != 0x1) {
        DEBUG ((EFI_D_INFO, "The slot at 0x%x is not embedded slot type\n", MmioBase[Index]));
        Status = EFI_UNSUPPORTED;
        continue;
      }

      Status = EmmcPeimHcReset (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }
      Status = EmmcPeimHcCardDetect (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }
      Status = EmmcPeimHcInitHost (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }

      SlotNum = Private->SlotNum;
      Slot    = &Private->Slot[SlotNum];
      CopyMem (Slot, &gEmmcHcSlotTemplate, sizeof (EMMC_PEIM_HC_SLOT));
      Slot->Private    = Private;
      Slot->EmmcHcBase = MmioBase[Index];
      CopyMem (&Slot->Capability, &Capability, sizeof (Capability));

      Status = EmmcPeimIdentification (Slot);
      if (EFI_ERROR (Status)) {
        continue;
      }

      ExtCsd = &Slot->ExtCsd;
      if (ExtCsd->ExtCsdRev < 5) {
        DEBUG ((EFI_D_ERROR, "The EMMC device version is too low, we don't support!!!\n"));
        Status = EFI_UNSUPPORTED;
        continue;
      }
      if ((ExtCsd->PartitioningSupport & BIT0) != BIT0) {
        DEBUG ((EFI_D_ERROR, "The EMMC device doesn't support Partition Feature!!!\n"));
        Status = EFI_UNSUPPORTED;
        continue;
      }

      for (PartitionIndex = 0; PartitionIndex < EMMC_PEIM_MAX_PARTITIONS; PartitionIndex++) {
        switch (PartitionIndex) {
          case EmmcPartitionUserData:
            SecCount = *(UINT32*)&ExtCsd->SecCount;
            Capacity = MultU64x32 ((UINT64)SecCount, 0x200);
            break;
          case EmmcPartitionBoot1:
          case EmmcPartitionBoot2:
            Capacity = ExtCsd->BootSizeMult * SIZE_128KB;
            break;
          case EmmcPartitionRPMB:
            Capacity = ExtCsd->RpmbSizeMult * SIZE_128KB;
            break;
          case EmmcPartitionGP1:
            GpSizeMult = (ExtCsd->GpSizeMult[0] | (ExtCsd->GpSizeMult[1] << 8) | (ExtCsd->GpSizeMult[2] << 16));
            Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
            break;
          case EmmcPartitionGP2:
            GpSizeMult = (ExtCsd->GpSizeMult[3] | (ExtCsd->GpSizeMult[4] << 8) | (ExtCsd->GpSizeMult[5] << 16));
            Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
            break;
          case EmmcPartitionGP3:
            GpSizeMult = (ExtCsd->GpSizeMult[6] | (ExtCsd->GpSizeMult[7] << 8) | (ExtCsd->GpSizeMult[8] << 16));
            Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
            break;
          case EmmcPartitionGP4:
            GpSizeMult = (ExtCsd->GpSizeMult[9] | (ExtCsd->GpSizeMult[10] << 8) | (ExtCsd->GpSizeMult[11] << 16));
            Capacity = MultU64x32 (MultU64x32 (MultU64x32 ((UINT64)GpSizeMult, ExtCsd->HcWpGrpSize), ExtCsd->HcEraseGrpSize), SIZE_512KB);
            break;
          default:
            ASSERT (FALSE);
            continue;
        }

        MediaNum = Slot->MediaNum;
        if (Capacity != 0) {
          Slot->Media[MediaNum].LastBlock = DivU64x32 (Capacity, Slot->Media[MediaNum].BlockSize) - 1;
          Slot->PartitionType[MediaNum] = PartitionIndex;
          Private->TotalBlkIoDevices++;
          Slot->MediaNum++;
        }
      }
      Private->SlotNum++;
    }
    Controller++;

    if (!EFI_ERROR (Status)) {
      PeiServicesInstallPpi (&Private->BlkIoPpiList);
      PeiServicesNotifyPpi (&Private->EndOfPeiNotifyList);
    } else {
      if (Private->Pool->Head != NULL) {
        EmmcPeimFreeMemPool (Private->Pool);
      }
    }
  }

  return EFI_SUCCESS;
}
