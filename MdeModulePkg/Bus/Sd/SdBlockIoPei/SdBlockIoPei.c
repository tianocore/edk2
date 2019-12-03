/** @file

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdBlockIoPei.h"

//
// Template for SD HC Slot Data.
//
SD_PEIM_HC_SLOT   gSdHcSlotTemplate = {
  SD_PEIM_SLOT_SIG,               // Signature
  {                               // Media
    MSG_SD_DP,
    FALSE,
    TRUE,
    FALSE,
    0x200,
    0
  },
  0,                              // SdHcBase
  {                               // Capability
    0,
  },
  {                               // Csd
    0,
  },
  TRUE,                           // SectorAddressing
  NULL                            // Private
};

//
// Template for SD HC Private Data.
//
SD_PEIM_HC_PRIVATE_DATA gSdHcPrivateTemplate = {
  SD_PEIM_SIG,                    // Signature
  NULL,                           // Pool
  {                               // BlkIoPpi
    SdBlockIoPeimGetDeviceNo,
    SdBlockIoPeimGetMediaInfo,
    SdBlockIoPeimReadBlocks
  },
  {                               // BlkIo2Ppi
    EFI_PEI_RECOVERY_BLOCK_IO2_PPI_REVISION,
    SdBlockIoPeimGetDeviceNo2,
    SdBlockIoPeimGetMediaInfo2,
    SdBlockIoPeimReadBlocks2
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
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    SdBlockIoPeimEndOfPei
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
SdBlockIoPeimGetDeviceNo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  SD_PEIM_HC_PRIVATE_DATA            *Private;

  Private = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);
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
SdBlockIoPeimGetMediaInfo (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO_PPI  *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO_MEDIA         *MediaInfo
  )
{
  SD_PEIM_HC_PRIVATE_DATA            *Private;

  Private   = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);

  if ((DeviceIndex == 0) || (DeviceIndex > Private->TotalBlkIoDevices) || (DeviceIndex > SD_PEIM_MAX_SLOTS)) {
    return EFI_INVALID_PARAMETER;
  }

  MediaInfo->DeviceType   = SD;
  MediaInfo->MediaPresent = TRUE;
  MediaInfo->LastBlock    = (UINTN)Private->Slot[DeviceIndex - 1].Media.LastBlock;
  MediaInfo->BlockSize    = Private->Slot[DeviceIndex - 1].Media.BlockSize;

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
SdBlockIoPeimReadBlocks (
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
  SD_PEIM_HC_PRIVATE_DATA            *Private;
  UINTN                              Remaining;
  UINT32                             MaxBlock;

  Status  = EFI_SUCCESS;
  Private = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS (This);

  //
  // Check parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if ((DeviceIndex == 0) || (DeviceIndex > Private->TotalBlkIoDevices) || (DeviceIndex > SD_PEIM_MAX_SLOTS)) {
    return EFI_INVALID_PARAMETER;
  }

  BlockSize = Private->Slot[DeviceIndex - 1].Media.BlockSize;
  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (StartLBA > Private->Slot[DeviceIndex - 1].Media.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  NumberOfBlocks = BufferSize / BlockSize;

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

    BufferSize = NumberOfBlocks * BlockSize;
    if (NumberOfBlocks != 1) {
      Status = SdPeimRwMultiBlocks (&Private->Slot[DeviceIndex - 1], StartLBA, BlockSize, Buffer, BufferSize, TRUE);
    } else {
      Status = SdPeimRwSingleBlock (&Private->Slot[DeviceIndex - 1], StartLBA, BlockSize, Buffer, BufferSize, TRUE);
    }
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
SdBlockIoPeimGetDeviceNo2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  OUT UINTN                          *NumberBlockDevices
  )
{
  SD_PEIM_HC_PRIVATE_DATA   *Private;

  Private = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);
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
SdBlockIoPeimGetMediaInfo2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  IN  UINTN                          DeviceIndex,
  OUT EFI_PEI_BLOCK_IO2_MEDIA        *MediaInfo
  )
{
  EFI_STATUS                         Status;
  SD_PEIM_HC_PRIVATE_DATA            *Private;
  EFI_PEI_BLOCK_IO_MEDIA             Media;

  Private = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);

  Status  = SdBlockIoPeimGetMediaInfo (
              PeiServices,
              &Private->BlkIoPpi,
              DeviceIndex,
              &Media
              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (MediaInfo, &(Private->Slot[DeviceIndex - 1].Media), sizeof (EFI_PEI_BLOCK_IO2_MEDIA));
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
SdBlockIoPeimReadBlocks2 (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  EFI_PEI_RECOVERY_BLOCK_IO2_PPI *This,
  IN  UINTN                          DeviceIndex,
  IN  EFI_PEI_LBA                    StartLBA,
  IN  UINTN                          BufferSize,
  OUT VOID                           *Buffer
  )
{
  EFI_STATUS                         Status;
  SD_PEIM_HC_PRIVATE_DATA            *Private;

  Status    = EFI_SUCCESS;
  Private   = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS2 (This);

  Status  = SdBlockIoPeimReadBlocks (
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
SdBlockIoPeimEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  SD_PEIM_HC_PRIVATE_DATA       *Private;

  Private = GET_SD_PEIM_HC_PRIVATE_DATA_FROM_THIS_NOTIFY (NotifyDescriptor);

  if ((Private->Pool != NULL) && (Private->Pool->Head != NULL)) {
    SdPeimFreeMemPool (Private->Pool);
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
InitializeSdBlockIoPeim (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                       Status;
  SD_PEIM_HC_PRIVATE_DATA          *Private;
  EDKII_SD_MMC_HOST_CONTROLLER_PPI *SdMmcHcPpi;
  UINT32                           Index;
  UINTN                            *MmioBase;
  UINT8                            BarNum;
  UINT8                            SlotNum;
  UINT8                            Controller;
  UINT64                           Capacity;
  SD_HC_SLOT_CAP                   Capability;
  SD_PEIM_HC_SLOT                  *Slot;
  SD_CSD                           *Csd;
  SD_CSD2                          *Csd2;
  UINT32                           CSize;
  UINT32                           CSizeMul;
  UINT32                           ReadBlLen;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  //
  // locate Sd host controller PPI
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

    Private = AllocateCopyPool (sizeof (SD_PEIM_HC_PRIVATE_DATA), &gSdHcPrivateTemplate);
    if (Private == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
    Private->BlkIoPpiList.Ppi  = (VOID*)&Private->BlkIoPpi;
    Private->BlkIo2PpiList.Ppi = (VOID*)&Private->BlkIo2Ppi;
    //
    // Initialize the memory pool which will be used in all transactions.
    //
    Status = SdPeimInitMemPool (Private);
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    for (Index = 0; Index < BarNum; Index++) {
      Status = SdPeimHcGetCapability (MmioBase[Index], &Capability);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (Capability.SlotType != 0x1) {
        DEBUG ((EFI_D_INFO, "The slot at 0x%x is not embedded slot type\n", MmioBase[Index]));
        Status = EFI_UNSUPPORTED;
        continue;
      }

      Status = SdPeimHcReset (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }
      Status = SdPeimHcCardDetect (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }
      Status = SdPeimHcInitHost (MmioBase[Index]);
      if (EFI_ERROR (Status)) {
        continue;
      }

      SlotNum = Private->SlotNum;
      Slot    = &Private->Slot[SlotNum];
      CopyMem (Slot, &gSdHcSlotTemplate, sizeof (SD_PEIM_HC_SLOT));
      Slot->Private  = Private;
      Slot->SdHcBase = MmioBase[Index];
      CopyMem (&Slot->Capability, &Capability, sizeof (Capability));

      Status = SdPeimIdentification (Slot);
      if (EFI_ERROR (Status)) {
        continue;
      }

      Csd = &Slot->Csd;
      if (Csd->CsdStructure == 0) {
        Slot->SectorAddressing = FALSE;
        CSize     = (Csd->CSizeHigh << 2 | Csd->CSizeLow) + 1;
        CSizeMul  = (1 << (Csd->CSizeMul + 2));
        ReadBlLen = (1 << (Csd->ReadBlLen));
        Capacity  = MultU64x32 (MultU64x32 ((UINT64)CSize, CSizeMul), ReadBlLen);
      } else {
        Slot->SectorAddressing = TRUE;
        Csd2     = (SD_CSD2*)(VOID*)Csd;
        CSize    = (Csd2->CSizeHigh << 16 | Csd2->CSizeLow) + 1;
        Capacity = MultU64x32 ((UINT64)CSize, SIZE_512KB);
      }

      Slot->Media.LastBlock = DivU64x32 (Capacity, Slot->Media.BlockSize) - 1;

      Private->TotalBlkIoDevices++;
      Private->SlotNum++;
    }

    Controller++;
    if (!EFI_ERROR (Status)) {
      PeiServicesInstallPpi (&Private->BlkIoPpiList);
      PeiServicesNotifyPpi (&Private->EndOfPeiNotifyList);
    } else {
      if (Private->Pool->Head != NULL) {
        SdPeimFreeMemPool (Private->Pool);
      }
    }
  }

  return EFI_SUCCESS;
}
