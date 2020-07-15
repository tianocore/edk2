/** @file
PEIM to produce gEfiPeiVirtualBlockIoPpiGuid & gEfiPeiVirtualBlockIo2PpiGuid PPI for
ATA controllers in the platform.

This PPI can be consumed by PEIM which produce gEfiPeiDeviceRecoveryModulePpiGuid
for Atapi CD ROM device.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AtapiPeim.h"

/**
  Initializes the Atapi Block Io PPI.

  @param[in]  FileHandle           Handle of the file being invoked.
  @param[in]  PeiServices          Describes the list of possible PEI Services.

  @retval     EFI_SUCCESS          Operation performed successfully.
  @retval     EFI_OUT_OF_RESOURCES Not enough memory to allocate.

**/
EFI_STATUS
EFIAPI
AtapiPeimEntry (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PEI_ATA_CONTROLLER_PPI  *AtaControllerPpi;
  EFI_STATUS              Status;
  ATAPI_BLK_IO_DEV        *AtapiBlkIoDev;

  Status = PeiServicesRegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiServicesLocatePpi (
              &gPeiAtaControllerPpiGuid,
              0,
              NULL,
              (VOID **) &AtaControllerPpi
              );
  ASSERT_EFI_ERROR (Status);

  AtapiBlkIoDev = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (*AtapiBlkIoDev)));
  if (AtapiBlkIoDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AtapiBlkIoDev->Signature        = ATAPI_BLK_IO_DEV_SIGNATURE;
  AtapiBlkIoDev->AtaControllerPpi = AtaControllerPpi;

  //
  // atapi device enumeration and build private data
  //
  AtapiEnumerateDevices (AtapiBlkIoDev);

  AtapiBlkIoDev->AtapiBlkIo.GetNumberOfBlockDevices = AtapiGetNumberOfBlockDevices;
  AtapiBlkIoDev->AtapiBlkIo.GetBlockDeviceMediaInfo = AtapiGetBlockDeviceMediaInfo;
  AtapiBlkIoDev->AtapiBlkIo.ReadBlocks              = AtapiReadBlocks;
  AtapiBlkIoDev->AtapiBlkIo2.Revision                = EFI_PEI_RECOVERY_BLOCK_IO2_PPI_REVISION;
  AtapiBlkIoDev->AtapiBlkIo2.GetNumberOfBlockDevices = AtapiGetNumberOfBlockDevices2;
  AtapiBlkIoDev->AtapiBlkIo2.GetBlockDeviceMediaInfo = AtapiGetBlockDeviceMediaInfo2;
  AtapiBlkIoDev->AtapiBlkIo2.ReadBlocks              = AtapiReadBlocks2;

  AtapiBlkIoDev->PpiDescriptor.Flags                = EFI_PEI_PPI_DESCRIPTOR_PPI;
  AtapiBlkIoDev->PpiDescriptor.Guid                 = &gEfiPeiVirtualBlockIoPpiGuid;
  AtapiBlkIoDev->PpiDescriptor.Ppi                  = &AtapiBlkIoDev->AtapiBlkIo;

  AtapiBlkIoDev->PpiDescriptor2.Flags                = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  AtapiBlkIoDev->PpiDescriptor2.Guid                 = &gEfiPeiVirtualBlockIo2PpiGuid;
  AtapiBlkIoDev->PpiDescriptor2.Ppi                  = &AtapiBlkIoDev->AtapiBlkIo2;

  DEBUG ((EFI_D_INFO, "Atatpi Device Count is %d\n", AtapiBlkIoDev->DeviceCount));
  if (AtapiBlkIoDev->DeviceCount != 0) {
    Status = PeiServicesInstallPpi (&AtapiBlkIoDev->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
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
  @param[in]  This                 Indicates the EFI_PEI_RECOVERY_BLOCK_IO_PPI
                                   instance.
  @param[out] NumberBlockDevices   The number of block I/O devices discovered.

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
AtapiGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI   *This,
  OUT  UINTN                             *NumberBlockDevices
  )
{
  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev;

  AtapiBlkIoDev = NULL;

  AtapiBlkIoDev       = PEI_RECOVERY_ATAPI_FROM_BLKIO_THIS (This);

  *NumberBlockDevices = AtapiBlkIoDev->DeviceCount;

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

  @retval EFI_SUCCESS           Media information about the specified block device
                                was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware
                                error.
  @retval Others                Other failure occurs.

**/
EFI_STATUS
EFIAPI
AtapiGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  )
{
  UINTN             DeviceCount;
  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev;
  EFI_STATUS        Status;
  UINTN             Index;

  AtapiBlkIoDev = NULL;

  if (This == NULL || MediaInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtapiBlkIoDev = PEI_RECOVERY_ATAPI_FROM_BLKIO_THIS (This);

  DeviceCount   = AtapiBlkIoDev->DeviceCount;

  //
  // DeviceIndex is a value from 1 to NumberBlockDevices.
  //
  if ((DeviceIndex < 1) || (DeviceIndex > DeviceCount) || (DeviceIndex > MAX_IDE_DEVICES)) {
    return EFI_INVALID_PARAMETER;
  }

  Index = DeviceIndex - 1;

  //
  // probe media and retrieve latest media information
  //
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo DevicePosition is %d\n", AtapiBlkIoDev->DeviceInfo[Index].DevicePosition));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo DeviceType is   %d\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.DeviceType));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo MediaPresent is %d\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.MediaPresent));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo BlockSize is  0x%x\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.BlockSize));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo LastBlock is  0x%x\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.LastBlock));

  Status = DetectMedia (
             AtapiBlkIoDev,
             AtapiBlkIoDev->DeviceInfo[Index].DevicePosition,
             &AtapiBlkIoDev->DeviceInfo[Index].MediaInfo,
             &AtapiBlkIoDev->DeviceInfo[Index].MediaInfo2
             );
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((EFI_D_INFO, "Atatpi GetInfo DevicePosition is %d\n", AtapiBlkIoDev->DeviceInfo[Index].DevicePosition));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo DeviceType is   %d\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.DeviceType));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo MediaPresent is %d\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.MediaPresent));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo BlockSize is  0x%x\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.BlockSize));
  DEBUG ((EFI_D_INFO, "Atatpi GetInfo LastBlock is  0x%x\n", AtapiBlkIoDev->DeviceInfo[Index].MediaInfo.LastBlock));

  //
  // Get media info from AtapiBlkIoDev
  //
  CopyMem (MediaInfo, &AtapiBlkIoDev->DeviceInfo[Index].MediaInfo, sizeof(EFI_PEI_BLOCK_IO_MEDIA));

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
                            PPIs will manage multiple block devices, the PPIs that
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
AtapiReadBlocks (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLBA,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  )
{

  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_STATUS              Status;
  UINTN                   NumberOfBlocks;
  UINTN                   BlockSize;
  ATAPI_BLK_IO_DEV        *AtapiBlkIoDev;

  AtapiBlkIoDev = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtapiBlkIoDev = PEI_RECOVERY_ATAPI_FROM_BLKIO_THIS (This);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Status = AtapiGetBlockDeviceMediaInfo (
            PeiServices,
            This,
            DeviceIndex,
            &MediaInfo
            );
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (!MediaInfo.MediaPresent) {
    return EFI_NO_MEDIA;
  }

  BlockSize = MediaInfo.BlockSize;

  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if ((StartLBA + NumberOfBlocks - 1) > AtapiBlkIoDev->DeviceInfo[DeviceIndex - 1].MediaInfo2.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ReadSectors (
            AtapiBlkIoDev,
            AtapiBlkIoDev->DeviceInfo[DeviceIndex - 1].DevicePosition,
            Buffer,
            StartLBA,
            NumberOfBlocks,
            BlockSize
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
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

  @retval     EFI_SUCCESS          Operation performed successfully.

**/
EFI_STATUS
EFIAPI
AtapiGetNumberOfBlockDevices2 (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI    *This,
  OUT  UINTN                             *NumberBlockDevices
  )
{
  EFI_STATUS        Status;
  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev;

  AtapiBlkIoDev = PEI_RECOVERY_ATAPI_FROM_BLKIO2_THIS (This);

  Status = AtapiGetNumberOfBlockDevices (
             PeiServices,
             &AtapiBlkIoDev->AtapiBlkIo,
             NumberBlockDevices
             );

  return Status;
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

  @retval EFI_SUCCESS           Media information about the specified block device
                                was obtained successfully.
  @retval EFI_DEVICE_ERROR      Cannot get the media information due to a hardware
                                error.
  @retval Others                Other failure occurs.

**/
EFI_STATUS
EFIAPI
AtapiGetBlockDeviceMediaInfo2 (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI       *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO2_MEDIA              *MediaInfo
  )
{
  ATAPI_BLK_IO_DEV           *AtapiBlkIoDev;
  EFI_STATUS                 Status;
  EFI_PEI_BLOCK_IO_MEDIA     Media;

  AtapiBlkIoDev = NULL;

  if (This == NULL || MediaInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtapiBlkIoDev = PEI_RECOVERY_ATAPI_FROM_BLKIO2_THIS (This);

  Status = AtapiGetBlockDeviceMediaInfo (
             PeiServices,
             &AtapiBlkIoDev->AtapiBlkIo,
             DeviceIndex,
             &Media
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get media info from AtapiBlkIoDev
  //
  CopyMem (MediaInfo, &AtapiBlkIoDev->DeviceInfo[DeviceIndex - 1].MediaInfo2, sizeof(EFI_PEI_BLOCK_IO2_MEDIA));

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
                            PPIs will manage multiple block devices, the PPIs that
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
AtapiReadBlocks2 (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO2_PPI    *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLBA,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  )
{
  EFI_STATUS          Status;
  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev;

  AtapiBlkIoDev = NULL;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AtapiBlkIoDev = PEI_RECOVERY_ATAPI_FROM_BLKIO2_THIS (This);

  Status = AtapiReadBlocks (
             PeiServices,
             &AtapiBlkIoDev->AtapiBlkIo,
             DeviceIndex,
             StartLBA,
             BufferSize,
             Buffer
             );

  return Status;
}


/**
  Enumerate Atapi devices.

  This function is used to enumerate Atatpi device in Ide channel.

  @param[in]  AtapiBlkIoDev  A pointer to atapi block IO device

**/
VOID
AtapiEnumerateDevices (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev
  )
{
  UINT8                   Index1;
  UINT8                   Index2;
  UINTN                   DevicePosition;
  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_PEI_BLOCK_IO2_MEDIA MediaInfo2;
  EFI_STATUS              Status;
  UINTN                   DeviceCount;
  UINT16                  CommandBlockBaseAddr;
  UINT16                  ControlBlockBaseAddr;
  UINT32                  IdeEnabledNumber;
  IDE_REGS_BASE_ADDR      IdeRegsBaseAddr[MAX_IDE_CHANNELS];

  DeviceCount = 0;
  DevicePosition = 0;

  //
  // Scan IDE bus for ATAPI devices
  //

  //
  // Enable Sata and IDE controller.
  //
  AtapiBlkIoDev->AtaControllerPpi->EnableAtaChannel (
                                  (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                                  AtapiBlkIoDev->AtaControllerPpi,
                                  PEI_ICH_IDE_PRIMARY | PEI_ICH_IDE_SECONDARY
                                  );

  //
  // Allow SATA Devices to spin-up. This is needed if
  // SEC and PEI phase is too short, for example Release Build.
  //
  DEBUG ((EFI_D_INFO, "Delay for %d seconds for SATA devices to spin-up\n", PcdGet16 (PcdSataSpinUpDelayInSecForRecoveryPath)));
  MicroSecondDelay (PcdGet16 (PcdSataSpinUpDelayInSecForRecoveryPath) * 1000 * 1000); //

  //
  // Get four channels (primary or secondary Pata, Sata Channel) Command and Control Regs Base address.
  //
  IdeEnabledNumber = AtapiBlkIoDev->AtaControllerPpi->GetIdeRegsBaseAddr (
                                                      (EFI_PEI_SERVICES **) GetPeiServicesTablePointer(),
                                                      AtapiBlkIoDev->AtaControllerPpi,
                                                      IdeRegsBaseAddr
                                                      );

  //
  // Using Command and Control Regs Base Address to fill other registers.
  //
  for (Index1 = 0; Index1 < IdeEnabledNumber; Index1 ++) {
    CommandBlockBaseAddr               = IdeRegsBaseAddr[Index1].CommandBlockBaseAddr;
    AtapiBlkIoDev->IdeIoPortReg[Index1].Data         = CommandBlockBaseAddr;
    AtapiBlkIoDev->IdeIoPortReg[Index1].Reg1.Feature = (UINT16) (CommandBlockBaseAddr + 0x1);
    AtapiBlkIoDev->IdeIoPortReg[Index1].SectorCount  = (UINT16) (CommandBlockBaseAddr + 0x2);
    AtapiBlkIoDev->IdeIoPortReg[Index1].SectorNumber = (UINT16) (CommandBlockBaseAddr + 0x3);
    AtapiBlkIoDev->IdeIoPortReg[Index1].CylinderLsb  = (UINT16) (CommandBlockBaseAddr + 0x4);
    AtapiBlkIoDev->IdeIoPortReg[Index1].CylinderMsb  = (UINT16) (CommandBlockBaseAddr + 0x5);
    AtapiBlkIoDev->IdeIoPortReg[Index1].Head         = (UINT16) (CommandBlockBaseAddr + 0x6);
    AtapiBlkIoDev->IdeIoPortReg[Index1].Reg.Command  = (UINT16) (CommandBlockBaseAddr + 0x7);

    ControlBlockBaseAddr                = IdeRegsBaseAddr[Index1].ControlBlockBaseAddr;
    AtapiBlkIoDev->IdeIoPortReg[Index1].Alt.DeviceControl = ControlBlockBaseAddr;
    AtapiBlkIoDev->IdeIoPortReg[Index1].DriveAddress      = (UINT16) (ControlBlockBaseAddr + 0x1);

    //
    // Scan IDE bus for ATAPI devices IDE or Sata device
    //
    for (Index2 = IdeMaster; Index2 < IdeMaxDevice; Index2++) {
      //
      // Pata & Sata, Primary & Secondary channel, Master & Slave device
      //
      DevicePosition = Index1 * 2 + Index2;

      if (DiscoverAtapiDevice (AtapiBlkIoDev, DevicePosition, &MediaInfo, &MediaInfo2)) {
        //
        // ATAPI Device at DevicePosition is found.
        //
        AtapiBlkIoDev->DeviceInfo[DeviceCount].DevicePosition = DevicePosition;
        //
        // Retrieve Media Info
        //
        Status  = DetectMedia (AtapiBlkIoDev, DevicePosition, &MediaInfo, &MediaInfo2);
        CopyMem (&(AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo), &MediaInfo, sizeof (MediaInfo));
        CopyMem (&(AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo2), &MediaInfo2, sizeof (MediaInfo2));

        DEBUG ((EFI_D_INFO, "Atatpi Device Position is %d\n", DevicePosition));
        DEBUG ((EFI_D_INFO, "Atatpi DeviceType is   %d\n", MediaInfo.DeviceType));
        DEBUG ((EFI_D_INFO, "Atatpi MediaPresent is %d\n", MediaInfo.MediaPresent));
        DEBUG ((EFI_D_INFO, "Atatpi BlockSize is  0x%x\n", MediaInfo.BlockSize));

        if (EFI_ERROR (Status)) {
          AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo.MediaPresent = FALSE;
          AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo.LastBlock    = 0;
          AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo2.MediaPresent = FALSE;
          AtapiBlkIoDev->DeviceInfo[DeviceCount].MediaInfo2.LastBlock    = 0;
        }
        DeviceCount += 1;
      }
    }
  }

  AtapiBlkIoDev->DeviceCount = DeviceCount;
}

/**
  Detect Atapi devices.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[out] MediaInfo       The media information of the specified block media.
  @param[out] MediaInfo2      The media information 2 of the specified block media.

  @retval TRUE                Atapi device exists in specified position.
  @retval FALSE               Atapi device does not exist in specified position.

**/
BOOLEAN
DiscoverAtapiDevice (
  IN  ATAPI_BLK_IO_DEV              *AtapiBlkIoDev,
  IN  UINTN                         DevicePosition,
  OUT EFI_PEI_BLOCK_IO_MEDIA        *MediaInfo,
  OUT EFI_PEI_BLOCK_IO2_MEDIA       *MediaInfo2
  )
{
  EFI_STATUS  Status;

  if (!DetectIDEController (AtapiBlkIoDev, DevicePosition)) {
    return FALSE;
  }
  //
  // test if it is an ATAPI device (only supported device)
  //
  if (ATAPIIdentify (AtapiBlkIoDev, DevicePosition) == EFI_SUCCESS) {

    Status = Inquiry (AtapiBlkIoDev, DevicePosition, MediaInfo, MediaInfo2);
    if (!EFI_ERROR (Status)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check power mode of Atapi devices.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in]  AtaCommand      The Ata Command passed in.

  @retval EFI_SUCCESS         The Atapi device support power mode.
  @retval EFI_NOT_FOUND       The Atapi device not found.
  @retval EFI_TIMEOUT         Atapi command transaction is time out.
  @retval EFI_ABORTED         Atapi command abort.

**/
EFI_STATUS
CheckPowerMode (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  UINTN               DevicePosition,
  IN  UINT8               AtaCommand
  )
{
  UINT8       Channel;
  UINT8       Device;
  UINT16      StatusRegister;
  UINT16      HeadRegister;
  UINT16      CommandRegister;
  UINT16      ErrorRegister;
  UINT16      SectorCountRegister;
  EFI_STATUS  Status;
  UINT8       StatusValue;
  UINT8       ErrorValue;
  UINT8       SectorCountValue;

  Channel             = (UINT8) (DevicePosition / 2);
  Device              = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  StatusRegister      = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Status;
  HeadRegister        = AtapiBlkIoDev->IdeIoPortReg[Channel].Head;
  CommandRegister     = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  ErrorRegister       = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg1.Error;
  SectorCountRegister = AtapiBlkIoDev->IdeIoPortReg[Channel].SectorCount;

  //
  // select device
  //
  IoWrite8 (HeadRegister, (UINT8) ((Device << 4) | 0xe0));

  //
  // refresh the SectorCount register
  //
  SectorCountValue = 0x55;
  IoWrite8 (SectorCountRegister, SectorCountValue);

  //
  // select device
  //
  IoWrite8 (HeadRegister, (UINT8) ((Device << 4) | 0xe0));

  Status = DRDYReady (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), 100);

  //
  // select device
  //
  IoWrite8 (HeadRegister, (UINT8) ((Device << 4) | 0xe0));
  //
  // send 'check power' commandd via Command Register
  //
  IoWrite8 (CommandRegister, AtaCommand);

  Status = WaitForBSYClear (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), 3000);
  if (EFI_ERROR (Status)) {
    return EFI_TIMEOUT;
  }

  StatusValue = IoRead8 (StatusRegister);

  //
  // command returned status is DRDY, indicating device supports the command,
  // so device is present.
  //
  if ((StatusValue & ATA_STSREG_DRDY) == ATA_STSREG_DRDY) {
    return EFI_SUCCESS;
  }

  SectorCountValue = IoRead8 (SectorCountRegister);

  //
  // command returned status is ERR & ABRT_ERR, indicating device does not support
  // the command, so device is present.
  //
  if ((StatusValue & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
    ErrorValue = IoRead8 (ErrorRegister);
    if ((ErrorValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
      return EFI_ABORTED;
    } else {
      //
      // According to spec, no other error code is valid
      //
      return EFI_NOT_FOUND;
    }
  }

  if ((SectorCountValue == 0x00) || (SectorCountValue == 0x80) || (SectorCountValue == 0xff)) {
    //
    // Write SectorCount 0x55 but return valid state value. Maybe no device
    // exists or some slow kind of ATAPI device exists.
    //
    IoWrite8 (HeadRegister, (UINT8) ((Device << 4) | 0xe0));

    //
    // write 0x55 and 0xaa to SectorCounter register,
    // if the data could be written into the register,
    // indicating the device is present, otherwise the device is not present.
    //
    SectorCountValue = 0x55;
    IoWrite8 (SectorCountRegister, SectorCountValue);
    MicroSecondDelay (10000);

    SectorCountValue = IoRead8 (SectorCountRegister);
    if (SectorCountValue != 0x55) {
      return EFI_NOT_FOUND;
    }
    //
    // Send a "ATAPI TEST UNIT READY" command ... slow but accurate
    //
    Status = TestUnitReady (AtapiBlkIoDev, DevicePosition);
    return Status;
  }

  return EFI_NOT_FOUND;
}

/**
  Detect if an IDE controller exists in specified position.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.

  @retval TRUE         The Atapi device exists.
  @retval FALSE        The Atapi device does not present.

**/
BOOLEAN
DetectIDEController (
  IN  ATAPI_BLK_IO_DEV   *AtapiBlkIoDev,
  IN  UINTN              DevicePosition
  )
{
  UINT8       Channel;
  EFI_STATUS  Status;
  UINT8       AtaCommand;

  Channel           = (UINT8) (DevicePosition / 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);
  //
  //  Wait 31 seconds for BSY clear
  //
  Status = WaitForBSYClear (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), 31000);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Send 'check power' command for IDE device
  //
  AtaCommand  = 0xE5;
  Status      = CheckPowerMode (AtapiBlkIoDev, DevicePosition, AtaCommand);
  if ((Status == EFI_ABORTED) || (Status == EFI_SUCCESS)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Wait specified time interval to poll for BSY bit clear in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        BSY bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        BSY bit is not cleared in the specified time interval.

**/
EFI_STATUS
WaitForBSYClear (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    StatusValue = IoRead8 (StatusRegister);
    if ((StatusValue & ATA_STSREG_BSY) == 0x00) {
      break;
    }
    MicroSecondDelay (250);

    Delay--;

  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRDY bit set in the Status register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRDY bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRDY bit is not set in the specified time interval.

**/
EFI_STATUS
DRDYReady (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    StatusValue = IoRead8 (StatusRegister);
    //
    //  BSY == 0 , DRDY == 1
    //
    if ((StatusValue & (ATA_STSREG_DRDY | ATA_STSREG_BSY)) == ATA_STSREG_DRDY) {
      break;
    }

  if ((StatusValue & (ATA_STSREG_ERR | ATA_STSREG_BSY)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;

  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit clear in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    StatusValue = IoRead8 (StatusRegister);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((StatusValue & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

  if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit clear in the Alternate Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is cleared in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not cleared in the specified time interval.

**/
EFI_STATUS
DRQClear2 (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  AltStatusRegister;
  UINT8   AltStatusValue;
  UINT8   ErrValue;

  AltStatusValue    = 0;

  AltStatusRegister = IdeIoRegisters->Alt.AltStatus;

  Delay             = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    AltStatusValue = IoRead8 (AltStatusRegister);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((AltStatusValue & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

  if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {
    ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
    if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
    return EFI_ABORTED;
    }
  }

    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit set in the Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  StatusRegister;
  UINT8   StatusValue;
  UINT8   ErrValue;

  StatusValue     = 0;
  ErrValue        = 0;

  StatusRegister  = IdeIoRegisters->Reg.Status;

  Delay           = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {
    //
    //  read Status Register will clear interrupt
    //
    StatusValue = IoRead8 (StatusRegister);

    //
    //  BSY==0,DRQ==1
    //
    if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((StatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
      if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }
    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Wait specified time interval to poll for DRQ bit set in the Alternate Status Register.

  @param[in]  AtapiBlkIoDev          A pointer to atapi block IO device.
  @param[in]  IdeIoRegisters         A pointer to IDE IO registers.
  @param[in]  TimeoutInMilliSeconds  Time specified in milliseconds.

  @retval EFI_SUCCESS        DRQ bit is set in the specified time interval.
  @retval EFI_TIMEOUT        DRQ bit is not set in the specified time interval.
  @retval EFI_ABORTED        Operation Aborted.

**/
EFI_STATUS
DRQReady2 (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  IDE_BASE_REGISTERS  *IdeIoRegisters,
  IN  UINTN               TimeoutInMilliSeconds
  )
{
  UINTN   Delay;
  UINT16  AltStatusRegister;
  UINT8   AltStatusValue;
  UINT8   ErrValue;

  AltStatusValue    = 0;

  AltStatusRegister = IdeIoRegisters->Alt.AltStatus;

  Delay             = ((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 250) + 1;
  do {

    AltStatusValue = IoRead8 (AltStatusRegister);

    //
    //  BSY==0,DRQ==1
    //
    if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((AltStatusValue & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrValue = IoRead8 (IdeIoRegisters->Reg1.Error);
      if ((ErrValue & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }
    MicroSecondDelay (250);

    Delay--;
  } while (Delay != 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Check if there is an error in Status Register.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  StatusReg         The address to IDE IO registers.

  @retval EFI_SUCCESS        Operation success.
  @retval EFI_DEVICE_ERROR   Device error.

**/
EFI_STATUS
CheckErrorStatus (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  UINT16              StatusReg
  )
{
  UINT8 StatusValue;

  StatusValue = IoRead8 (StatusReg);

  if ((StatusValue & (ATA_STSREG_ERR | ATA_STSREG_DWF | ATA_STSREG_CORR)) == 0) {

    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;

}

/**
  Idendify Atapi devices.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.

  @retval EFI_SUCCESS        Identify successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be identified successfully.

**/
EFI_STATUS
ATAPIIdentify (
  IN  ATAPI_BLK_IO_DEV        *AtapiBlkIoDev,
  IN  UINTN                   DevicePosition
  )
{
  ATAPI_IDENTIFY_DATA  AtapiIdentifyData;
  UINT8                Channel;
  UINT8                Device;
  UINT16               StatusReg;
  UINT16               HeadReg;
  UINT16               CommandReg;
  UINT16               DataReg;
  UINT16               SectorCountReg;
  UINT16               SectorNumberReg;
  UINT16               CylinderLsbReg;
  UINT16               CylinderMsbReg;

  UINT32               WordCount;
  UINT32               Increment;
  UINT32               Index;
  UINT32               ByteCount;
  UINT16               *Buffer16;

  EFI_STATUS           Status;

  ByteCount       = sizeof (AtapiIdentifyData);
  Buffer16        = (UINT16 *) &AtapiIdentifyData;

  Channel         = (UINT8) (DevicePosition / 2);
  Device          = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  StatusReg       = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Status;
  HeadReg         = AtapiBlkIoDev->IdeIoPortReg[Channel].Head;
  CommandReg      = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  DataReg         = AtapiBlkIoDev->IdeIoPortReg[Channel].Data;
  SectorCountReg  = AtapiBlkIoDev->IdeIoPortReg[Channel].SectorCount;
  SectorNumberReg = AtapiBlkIoDev->IdeIoPortReg[Channel].SectorNumber;
  CylinderLsbReg  = AtapiBlkIoDev->IdeIoPortReg[Channel].CylinderLsb;
  CylinderMsbReg  = AtapiBlkIoDev->IdeIoPortReg[Channel].CylinderMsb;

  //
  // Send ATAPI Identify Command to get IDENTIFY data.
  //
  if (WaitForBSYClear (
        AtapiBlkIoDev,
        &(AtapiBlkIoDev->IdeIoPortReg[Channel]),
        ATATIMEOUT
        ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // select device via Head/Device register.
  // Before write Head/Device register, BSY and DRQ must be 0.
  //
  if (DRQClear2 (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), ATATIMEOUT) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  //  e0:1110,0000-- bit7 and bit5 are reserved bits.
  //           bit6 set means LBA mode
  //
  IoWrite8 (HeadReg, (UINT8) ((Device << 4) | 0xe0));

  //
  // set all the command parameters
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  if (DRQClear2 (
        AtapiBlkIoDev,
        &(AtapiBlkIoDev->IdeIoPortReg[Channel]),
        ATATIMEOUT
        ) != EFI_SUCCESS) {

    return EFI_DEVICE_ERROR;
  }

  IoWrite8 (SectorCountReg, 0);
  IoWrite8 (SectorNumberReg, 0);
  IoWrite8 (CylinderLsbReg, 0);
  IoWrite8 (CylinderMsbReg, 0);

  //
  // send command via Command Register
  //
  IoWrite8 (CommandReg, ATA_CMD_IDENTIFY_DEVICE);

  //
  // According to PIO data in protocol, host can perform a series of reads to the
  // data register after each time device set DRQ ready;
  // The data size of "a series of read" is command specific.
  // For most ATA command, data size received from device will not exceed 1 sector,
  // hense the data size for "a series of read" can be the whole data size of one command request.
  // For ATA command such as Read Sector command, whole data size of one ATA command request is often larger
  // than 1 sector, according to the Read Sector command, the data size of "a series of read" is exactly
  // 1 sector.
  // Here for simplification reason, we specify the data size for "a series of read" to
  // 1 sector (256 words) if whole data size of one ATA commmand request is larger than 256 words.
  //
  Increment = 256;
  //
  // 256 words
  //
  WordCount = 0;
  //
  // WordCount is used to record bytes of currently transfered data
  //
  while (WordCount < ByteCount / 2) {
    //
    // Poll DRQ bit set, data transfer can be performed only when DRQ is ready.
    //
    Status = DRQReady2 (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), ATATIMEOUT);
    if (Status != EFI_SUCCESS) {
      return Status;
    }

    if (CheckErrorStatus (AtapiBlkIoDev, StatusReg) != EFI_SUCCESS) {

      return EFI_DEVICE_ERROR;
    }
    //
    // Get the byte count for one series of read
    //
    if ((WordCount + Increment) > ByteCount / 2) {
      Increment = ByteCount / 2 - WordCount;
    }
    //
    // perform a series of read without check DRQ ready
    //
    for (Index = 0; Index < Increment; Index++) {
      *Buffer16++ = IoRead16 (DataReg);
    }

    WordCount += Increment;

  }
  //
  // while
  //
  if (DRQClear (
        AtapiBlkIoDev,
        &(AtapiBlkIoDev->IdeIoPortReg[Channel]),
        ATATIMEOUT
        ) != EFI_SUCCESS) {
    return CheckErrorStatus (AtapiBlkIoDev, StatusReg);
  }

  return EFI_SUCCESS;

}

/**
  Sends out ATAPI Test Unit Ready Packet Command to the specified device
  to find out whether device is accessible.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.

  @retval EFI_SUCCESS        TestUnit command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed TestUnit command successfully.

**/
EFI_STATUS
TestUnitReady (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  UINTN               DevicePosition
  )
{
  ATAPI_PACKET_COMMAND  Packet;
  EFI_STATUS            Status;

  //
  // fill command packet
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.TestUnitReady.opcode = ATA_CMD_TEST_UNIT_READY;

  //
  // send command packet
  //
  Status = AtapiPacketCommandIn (AtapiBlkIoDev, DevicePosition, &Packet, NULL, 0, ATAPITIMEOUT);
  return Status;
}

/**
  Send out ATAPI commands conforms to the Packet Command with PIO Data In Protocol.

  @param[in]  AtapiBlkIoDev         A pointer to atapi block IO device.
  @param[in]  DevicePosition        An integer to signify device position.
  @param[in]  Packet                A pointer to ATAPI command packet.
  @param[in]  Buffer                Buffer to contain requested transfer data from device.
  @param[in]  ByteCount             Requested transfer data length.
  @param[in]  TimeoutInMilliSeconds Time out value, in unit of milliseconds.

  @retval EFI_SUCCESS        Command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed command successfully.

**/
EFI_STATUS
AtapiPacketCommandIn (
  IN  ATAPI_BLK_IO_DEV      *AtapiBlkIoDev,
  IN  UINTN                 DevicePosition,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeoutInMilliSeconds
  )
{
  UINT8       Channel;
  UINT8       Device;
  UINT16      StatusReg;
  UINT16      HeadReg;
  UINT16      CommandReg;
  UINT16      FeatureReg;
  UINT16      CylinderLsbReg;
  UINT16      CylinderMsbReg;
  UINT16      DeviceControlReg;
  UINT16      DataReg;
  EFI_STATUS  Status;
  UINT32      Count;
  UINT16      *CommandIndex;
  UINT16      *PtrBuffer;
  UINT32      Index;
  UINT8       StatusValue;
  UINT32      WordCount;

  //
  // required transfer data in word unit.
  //
  UINT32      RequiredWordCount;

  //
  // actual transfer data in word unit.
  //
  UINT32      ActualWordCount;

  Channel           = (UINT8) (DevicePosition / 2);
  Device            = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  StatusReg         = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Status;
  HeadReg           = AtapiBlkIoDev->IdeIoPortReg[Channel].Head;
  CommandReg        = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  FeatureReg        = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg1.Feature;
  CylinderLsbReg    = AtapiBlkIoDev->IdeIoPortReg[Channel].CylinderLsb;
  CylinderMsbReg    = AtapiBlkIoDev->IdeIoPortReg[Channel].CylinderMsb;
  DeviceControlReg  = AtapiBlkIoDev->IdeIoPortReg[Channel].Alt.DeviceControl;
  DataReg           = AtapiBlkIoDev->IdeIoPortReg[Channel].Data;

  //
  // Set all the command parameters by fill related registers.
  // Before write to all the following registers, BSY and DRQ must be 0.
  //
  if (DRQClear2 (
        AtapiBlkIoDev,
        &(AtapiBlkIoDev->IdeIoPortReg[Channel]),
        ATATIMEOUT
        ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Select device via Device/Head Register.
  // DEFAULT_CMD: 0xa0 (1010,0000)
  //
  IoWrite8 (HeadReg, (UINT8) ((Device << 4) | ATA_DEFAULT_CMD));

  //
  // No OVL; No DMA
  //
  IoWrite8 (FeatureReg, 0x00);

  //
  // set the transfersize to MAX_ATAPI_BYTE_COUNT to let the device
  // determine how many data should be transfered.
  //
  IoWrite8 (CylinderLsbReg, (UINT8) (ATAPI_MAX_BYTE_COUNT & 0x00ff));
  IoWrite8 (CylinderMsbReg, (UINT8) (ATAPI_MAX_BYTE_COUNT >> 8));

  //
  //  DEFAULT_CTL:0x0a (0000,1010)
  //  Disable interrupt
  //
  IoWrite8 (DeviceControlReg, ATA_DEFAULT_CTL);

  //
  // Send Packet command to inform device
  // that the following data bytes are command packet.
  //
  IoWrite8 (CommandReg, ATA_CMD_PACKET);

  Status = DRQReady (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), TimeoutInMilliSeconds);
  if (Status != EFI_SUCCESS) {
    return Status;
  }
  //
  // Send out command packet
  //
  CommandIndex = Packet->Data16;
  for (Count = 0; Count < 6; Count++, CommandIndex++) {
    IoWrite16 (DataReg, *CommandIndex);
    MicroSecondDelay (10);
  }

  StatusValue = IoRead8 (StatusReg);
  if ((StatusValue & ATA_STSREG_ERR) == ATA_STSREG_ERR) {
    //
    // Trouble! Something's wrong here... Wait some time and return. 3 second is
    // supposed to be long enough for a device reset latency or error recovery
    //
    MicroSecondDelay (3000000);
    return EFI_DEVICE_ERROR;
  }

  if (Buffer == NULL || ByteCount == 0) {
    return EFI_SUCCESS;
  }
  //
  // call PioReadWriteData() function to get
  // requested transfer data form device.
  //
  PtrBuffer         = Buffer;
  RequiredWordCount = ByteCount / 2;
  //
  // ActuralWordCount means the word count of data really transfered.
  //
  ActualWordCount = 0;

  Status          = EFI_SUCCESS;
  while ((Status == EFI_SUCCESS) && (ActualWordCount < RequiredWordCount)) {
    //
    // before each data transfer stream, the host should poll DRQ bit ready,
    // which informs device is ready to transfer data.
    //
    if (DRQReady2 (
          AtapiBlkIoDev,
          &(AtapiBlkIoDev->IdeIoPortReg[Channel]),
          TimeoutInMilliSeconds
          ) != EFI_SUCCESS) {
      return CheckErrorStatus (AtapiBlkIoDev, StatusReg);
    }
    //
    // read Status Register will clear interrupt
    //
    StatusValue = IoRead8 (StatusReg);

    //
    // get current data transfer size from Cylinder Registers.
    //
    WordCount = IoRead8 (CylinderMsbReg) << 8;
    WordCount = WordCount | IoRead8 (CylinderLsbReg);
    WordCount = WordCount & 0xffff;
    WordCount /= 2;

    //
    // perform a series data In/Out.
    //
    for (Index = 0; (Index < WordCount) && (ActualWordCount < RequiredWordCount); Index++, ActualWordCount++) {

      *PtrBuffer = IoRead16 (DataReg);

      PtrBuffer++;

    }

    if (((ATAPI_REQUEST_SENSE_CMD *) Packet)->opcode == ATA_CMD_REQUEST_SENSE && ActualWordCount >= 4) {
      RequiredWordCount = MIN (
                            RequiredWordCount,
                            (UINT32) (4 + (((ATAPI_REQUEST_SENSE_DATA *) Buffer)->addnl_sense_length / 2))
                            );
    }

  }
  //
  // After data transfer is completed, normally, DRQ bit should clear.
  //
  Status = DRQClear2 (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), TimeoutInMilliSeconds);
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // read status register to check whether error happens.
  //
  Status = CheckErrorStatus (AtapiBlkIoDev, StatusReg);
  return Status;
}

/**
  Sends out ATAPI Inquiry Packet Command to the specified device.
  This command will return INQUIRY data of the device.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[out] MediaInfo       The media information of the specified block media.
  @param[out] MediaInfo2      The media information 2 of the specified block media.

  @retval EFI_SUCCESS        Command executed successfully.
  @retval EFI_DEVICE_ERROR   Device cannot be executed command successfully.
  @retval EFI_UNSUPPORTED    Unsupported device type.

**/
EFI_STATUS
Inquiry (
  IN  ATAPI_BLK_IO_DEV              *AtapiBlkIoDev,
  IN  UINTN                         DevicePosition,
  OUT EFI_PEI_BLOCK_IO_MEDIA        *MediaInfo,
  OUT EFI_PEI_BLOCK_IO2_MEDIA       *MediaInfo2
  )
{
  ATAPI_PACKET_COMMAND        Packet;
  EFI_STATUS                  Status;
  ATAPI_INQUIRY_DATA          Idata;

  //
  // prepare command packet for the ATAPI Inquiry Packet Command.
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  ZeroMem (&Idata, sizeof (ATAPI_INQUIRY_DATA));

  Packet.Inquiry.opcode             = ATA_CMD_INQUIRY;
  Packet.Inquiry.page_code          = 0;
  Packet.Inquiry.allocation_length  = (UINT8) sizeof (ATAPI_INQUIRY_DATA);

  //
  // Send command packet and get requested Inquiry data.
  //
  Status = AtapiPacketCommandIn (
            AtapiBlkIoDev,
            DevicePosition,
            &Packet,
            (UINT16 *) (&Idata),
            sizeof (ATAPI_INQUIRY_DATA),
            ATAPITIMEOUT
            //50
            );

  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Identify device type via INQUIRY data.
  //
  switch (Idata.peripheral_type & 0x1f) {
  case 0x00:
    //
    // Magnetic Disk
    //
    MediaInfo->DeviceType   = IdeLS120;
    MediaInfo->MediaPresent = FALSE;
    MediaInfo->LastBlock    = 0;
    MediaInfo->BlockSize    = 0x200;
    MediaInfo2->InterfaceType  = MSG_ATAPI_DP;
    MediaInfo2->RemovableMedia = TRUE;
    MediaInfo2->MediaPresent   = FALSE;
    MediaInfo2->ReadOnly       = FALSE;
    MediaInfo2->BlockSize      = 0x200;
    MediaInfo2->LastBlock      = 0;
    break;

  case 0x05:
    //
    // CD-ROM
    //
    MediaInfo->DeviceType   = IdeCDROM;
    MediaInfo->MediaPresent = FALSE;
    MediaInfo->LastBlock    = 0;
    MediaInfo->BlockSize    = 0x800;
    MediaInfo2->InterfaceType  = MSG_ATAPI_DP;
    MediaInfo2->RemovableMedia = TRUE;
    MediaInfo2->MediaPresent   = FALSE;
    MediaInfo2->ReadOnly       = TRUE;
    MediaInfo2->BlockSize      = 0x200;
    MediaInfo2->LastBlock      = 0;
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Used before read/write blocks from/to ATAPI device media.
  Since ATAPI device media is removable, it is necessary to detect
  whether media is present and get current present media's information.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in, out] MediaInfo     The media information of the specified block media.
  @param[in, out] MediaInfo2    The media information 2 of the specified block media.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate required resources.

**/
EFI_STATUS
DetectMedia (
  IN  ATAPI_BLK_IO_DEV              *AtapiBlkIoDev,
  IN  UINTN                         DevicePosition,
  IN OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo,
  IN OUT EFI_PEI_BLOCK_IO2_MEDIA    *MediaInfo2
  )
{

  UINTN                     Index;
  UINTN                     RetryNum;
  UINTN                     MaxRetryNum;
  ATAPI_REQUEST_SENSE_DATA  *SenseBuffers;
  BOOLEAN                   NeedReadCapacity;
  BOOLEAN                   NeedRetry;
  EFI_STATUS                Status;
  UINT8                     SenseCounts;

  SenseBuffers = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (*SenseBuffers)));
  if (SenseBuffers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Test Unit Ready command is used to detect whether device is accessible,
  // the device will produce corresponding Sense data.
  //
  for (Index = 0; Index < 2; Index++) {

    Status = TestUnitReady (AtapiBlkIoDev, DevicePosition);
    if (Status != EFI_SUCCESS) {
      Status = ResetDevice (AtapiBlkIoDev, DevicePosition, FALSE);

      if (Status != EFI_SUCCESS) {
        ResetDevice (AtapiBlkIoDev, DevicePosition, TRUE);
      }

    } else {
      break;
    }
  }

  SenseCounts       = MAX_SENSE_KEY_COUNT;
  Status            = EFI_SUCCESS;
  NeedReadCapacity  = TRUE;

  for (Index = 0; Index < 5; Index++) {
    SenseCounts = MAX_SENSE_KEY_COUNT;
    Status = RequestSense (
              AtapiBlkIoDev,
              DevicePosition,
              SenseBuffers,
              &SenseCounts
              );
    DEBUG ((EFI_D_INFO, "Atapi Request Sense Count is %d\n", SenseCounts));
    if (IsDeviceStateUnclear (SenseBuffers, SenseCounts) || IsNoMedia (SenseBuffers, SenseCounts)) {
      //
      // We are not sure whether the media is present or not, try again
      //
      TestUnitReady (AtapiBlkIoDev, DevicePosition);
    } else {
      break;
    }
  }

  if (Status == EFI_SUCCESS) {

    if (IsNoMedia (SenseBuffers, SenseCounts)) {

      NeedReadCapacity        = FALSE;
      MediaInfo->MediaPresent = FALSE;
      MediaInfo->LastBlock    = 0;
      MediaInfo2->MediaPresent = FALSE;
      MediaInfo2->LastBlock    = 0;
    }

    if (IsMediaError (SenseBuffers, SenseCounts)) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (NeedReadCapacity) {
    //
    // at most retry 5 times
    //
    MaxRetryNum = 5;
    RetryNum    = 1;
    //
    // initial retry once
    //
    for (Index = 0; (Index < RetryNum) && (Index < MaxRetryNum); Index++) {

      Status = ReadCapacity (AtapiBlkIoDev, DevicePosition, MediaInfo, MediaInfo2);
      MicroSecondDelay (200000);
      SenseCounts = MAX_SENSE_KEY_COUNT;

      if (Status != EFI_SUCCESS) {

        Status = RequestSense (AtapiBlkIoDev, DevicePosition, SenseBuffers, &SenseCounts);
        //
        // If Request Sense data failed, reset the device and retry.
        //
        if (Status != EFI_SUCCESS) {

          Status = ResetDevice (AtapiBlkIoDev, DevicePosition, FALSE);
          //
          // if ATAPI soft reset fail,
          // use stronger reset mechanism -- ATA soft reset.
          //
          if (Status != EFI_SUCCESS) {
            ResetDevice (AtapiBlkIoDev, DevicePosition, TRUE);
          }

          RetryNum++;
          //
          // retry once more
          //
          continue;
        }
        //
        // No Media
        //
        if (IsNoMedia (SenseBuffers, SenseCounts)) {

          MediaInfo->MediaPresent = FALSE;
          MediaInfo->LastBlock    = 0;
          MediaInfo2->MediaPresent = FALSE;
          MediaInfo2->LastBlock    = 0;
          break;
        }

        if (IsMediaError (SenseBuffers, SenseCounts)) {
          return EFI_DEVICE_ERROR;
        }

        if (!IsDriveReady (SenseBuffers, SenseCounts, &NeedRetry)) {
          //
          // Drive not ready: if NeedRetry, then retry once more;
          // else return error
          //
          if (NeedRetry) {
            RetryNum++;
            continue;
          } else {
            return EFI_DEVICE_ERROR;
          }
        }
        //
        // if read capacity fail not for above reasons, retry once more
        //
        RetryNum++;

      }

    }

  }

  return EFI_SUCCESS;
}

/**
  Reset specified Atapi device.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in]  Extensive         If TRUE, use ATA soft reset, otherwise use Atapi soft reset.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ResetDevice (
  IN  ATAPI_BLK_IO_DEV  *AtapiBlkIoDev,
  IN  UINTN             DevicePosition,
  IN  BOOLEAN           Extensive
  )
{
  UINT8   DevControl;
  UINT8   Command;
  UINT8   DeviceSelect;
  UINT16  DeviceControlReg;
  UINT16  CommandReg;
  UINT16  HeadReg;
  UINT8   Channel;
  UINT8   Device;

  Channel           = (UINT8) (DevicePosition / 2);
  Device            = (UINT8) (DevicePosition % 2);

  ASSERT (Channel < MAX_IDE_CHANNELS);

  DeviceControlReg  = AtapiBlkIoDev->IdeIoPortReg[Channel].Alt.DeviceControl;
  CommandReg        = AtapiBlkIoDev->IdeIoPortReg[Channel].Reg.Command;
  HeadReg           = AtapiBlkIoDev->IdeIoPortReg[Channel].Head;

  if (Extensive) {

    DevControl = 0;
    DevControl |= ATA_CTLREG_SRST;
    //
    // set SRST bit to initiate soft reset
    //
    DevControl |= BIT1;
    //
    // disable Interrupt
    //
    IoWrite8 (DeviceControlReg, DevControl);

    //
    // Wait 10us
    //
    MicroSecondDelay (10);

    //
    // Clear SRST bit
    //
    DevControl &= 0xfb;
    //
    // 0xfb:1111,1011
    //
    IoWrite8 (DeviceControlReg, DevControl);

    //
    // slave device needs at most 31s to clear BSY
    //
    if (WaitForBSYClear (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), 31000) == EFI_TIMEOUT) {
      return EFI_DEVICE_ERROR;
    }

  } else {
    //
    // for ATAPI device, no need to wait DRDY ready after device selecting.
    // bit7 and bit5 are both set to 1 for backward compatibility
    //
    DeviceSelect = (UINT8) (((BIT7 | BIT5) | (Device << 4)));
    IoWrite8 (HeadReg, DeviceSelect);

    Command = ATA_CMD_SOFT_RESET;
    IoWrite8 (CommandReg, Command);

    //
    // BSY cleared is the only status return to the host by the device when reset is completed
    // slave device needs at most 31s to clear BSY
    //
    if (WaitForBSYClear (AtapiBlkIoDev, &(AtapiBlkIoDev->IdeIoPortReg[Channel]), 31000) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // stall 5 seconds to make the device status stable
    //
    MicroSecondDelay (STALL_1_SECONDS * 5);
  }

  return EFI_SUCCESS;

}

/**
  Sends out ATAPI Request Sense Packet Command to the specified device.

  @param[in]      AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]      DevicePosition  An integer to signify device position.
  @param[in]      SenseBuffers    Pointer to sense buffer.
  @param[in, out] SenseCounts     Length of sense buffer.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
RequestSense (
  IN  ATAPI_BLK_IO_DEV           *AtapiBlkIoDev,
  IN  UINTN                      DevicePosition,
  IN  ATAPI_REQUEST_SENSE_DATA   *SenseBuffers,
  IN  OUT  UINT8                 *SenseCounts
  )
{
  EFI_STATUS            Status;
  ATAPI_REQUEST_SENSE_DATA    *Sense;
  UINT16                *Ptr;
  BOOLEAN               SenseReq;
  ATAPI_PACKET_COMMAND  Packet;

  ZeroMem (SenseBuffers, sizeof (ATAPI_REQUEST_SENSE_DATA) * (*SenseCounts));
  //
  // fill command packet for Request Sense Packet Command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Packet.RequestSence.opcode            = ATA_CMD_REQUEST_SENSE;
  Packet.RequestSence.allocation_length = (UINT8) sizeof (ATAPI_REQUEST_SENSE_DATA);

  Ptr = (UINT16 *) SenseBuffers;
  //
  // initialize pointer
  //
  *SenseCounts = 0;
  //
  //  request sense data from device continiously until no sense data exists in the device.
  //
  for (SenseReq = TRUE; SenseReq;) {

    Sense = (ATAPI_REQUEST_SENSE_DATA *) Ptr;

    //
    // send out Request Sense Packet Command and get one Sense data form device
    //
    Status = AtapiPacketCommandIn (
              AtapiBlkIoDev,
              DevicePosition,
              &Packet,
              Ptr,
              sizeof (ATAPI_REQUEST_SENSE_DATA),
              ATAPITIMEOUT
              );
    //
    // failed to get Sense data
    //
    if (Status != EFI_SUCCESS) {
      if (*SenseCounts == 0) {
        return EFI_DEVICE_ERROR;
      } else {
        return EFI_SUCCESS;
      }
    }

    (*SenseCounts)++;

    if (*SenseCounts > MAX_SENSE_KEY_COUNT) {
      return EFI_SUCCESS;
    }
    //
    // We limit MAX sense data count to 20 in order to avoid dead loop. Some
    // incompatible ATAPI devices don't retrive NO_SENSE when there is no media.
    // In this case, dead loop occurs if we don't have a gatekeeper. 20 is
    // supposed to be large enough for any ATAPI device.
    //
    if ((Sense->sense_key != ATA_SK_NO_SENSE) && ((*SenseCounts) < 20)) {

      Ptr += sizeof (ATAPI_REQUEST_SENSE_DATA) / 2;
      //
      // Ptr is word based pointer
      //
    } else {
      //
      // when no sense key, skip out the loop
      //
      SenseReq = FALSE;
    }
  }

  return EFI_SUCCESS;
}

/**
  Sends out ATAPI Read Capacity Packet Command to the specified device.
  This command will return the information regarding the capacity of the
  media in the device.

  @param[in]  AtapiBlkIoDev     A pointer to atapi block IO device.
  @param[in]  DevicePosition    An integer to signify device position.
  @param[in, out] MediaInfo     The media information of the specified block media.
  @param[in, out] MediaInfo2    The media information 2 of the specified block media.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ReadCapacity (
  IN  ATAPI_BLK_IO_DEV              *AtapiBlkIoDev,
  IN  UINTN                         DevicePosition,
  IN OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo,
  IN OUT EFI_PEI_BLOCK_IO2_MEDIA    *MediaInfo2
  )
{
  EFI_STATUS                Status;
  ATAPI_PACKET_COMMAND      Packet;

  //
  // used for capacity data returned from ATAPI device
  //
  ATAPI_READ_CAPACITY_DATA        Data;
  ATAPI_READ_FORMAT_CAPACITY_DATA FormatData;

  ZeroMem (&Data, sizeof (Data));
  ZeroMem (&FormatData, sizeof (FormatData));

  if (MediaInfo->DeviceType == IdeCDROM) {

    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.Inquiry.opcode = ATA_CMD_READ_CAPACITY;
    Status = AtapiPacketCommandIn (
              AtapiBlkIoDev,
              DevicePosition,
              &Packet,
              (UINT16 *) (&Data),
              sizeof (ATAPI_READ_CAPACITY_DATA),
              ATAPITIMEOUT
              );

  } else {
    //
    // DeviceType == IdeLS120
    //
    ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
    Packet.ReadFormatCapacity.opcode                = ATA_CMD_READ_FORMAT_CAPACITY;
    Packet.ReadFormatCapacity.allocation_length_lo  = 12;
    Status = AtapiPacketCommandIn (
              AtapiBlkIoDev,
              DevicePosition,
              &Packet,
              (UINT16 *) (&FormatData),
              sizeof (ATAPI_READ_FORMAT_CAPACITY_DATA),
              ATAPITIMEOUT*10
              );
  }

  if (Status == EFI_SUCCESS) {

    if (MediaInfo->DeviceType == IdeCDROM) {

      MediaInfo->LastBlock    = ((UINT32) Data.LastLba3 << 24) | (Data.LastLba2 << 16) | (Data.LastLba1 << 8) | Data.LastLba0;
      MediaInfo->MediaPresent = TRUE;
      //
      // Because the user data portion in the sector of the Data CD supported
      // is always 800h
      //
      MediaInfo->BlockSize     = 0x800;

      MediaInfo2->LastBlock    = MediaInfo->LastBlock;
      MediaInfo2->MediaPresent = MediaInfo->MediaPresent;
      MediaInfo2->BlockSize    = (UINT32)MediaInfo->BlockSize;
    }

    if (MediaInfo->DeviceType == IdeLS120) {

      if (FormatData.DesCode == 3) {
        MediaInfo->MediaPresent = FALSE;
        MediaInfo->LastBlock    = 0;
        MediaInfo2->MediaPresent = FALSE;
        MediaInfo2->LastBlock    = 0;
      } else {
        MediaInfo->LastBlock = ((UINT32) FormatData.LastLba3 << 24) |
          (FormatData.LastLba2 << 16) |
          (FormatData.LastLba1 << 8) |
          FormatData.LastLba0;
        MediaInfo->LastBlock--;

        MediaInfo->MediaPresent = TRUE;

        MediaInfo->BlockSize    = 0x200;

        MediaInfo2->LastBlock    = MediaInfo->LastBlock;
        MediaInfo2->MediaPresent = MediaInfo->MediaPresent;
        MediaInfo2->BlockSize    = (UINT32)MediaInfo->BlockSize;

      }
    }

    return EFI_SUCCESS;

  } else {
    return EFI_DEVICE_ERROR;
  }
}

/**
  Perform read from disk in block unit.

  @param[in]  AtapiBlkIoDev   A pointer to atapi block IO device.
  @param[in]  DevicePosition  An integer to signify device position.
  @param[in]  Buffer          Buffer to contain read data.
  @param[in]  StartLba        Starting LBA address.
  @param[in]  NumberOfBlocks  Number of blocks to read.
  @param[in]  BlockSize       Size of each block.

  @retval EFI_SUCCESS           Command executed successfully.
  @retval EFI_DEVICE_ERROR      Some device errors happen.

**/
EFI_STATUS
ReadSectors (
  IN  ATAPI_BLK_IO_DEV    *AtapiBlkIoDev,
  IN  UINTN               DevicePosition,
  IN  VOID                *Buffer,
  IN  EFI_PEI_LBA         StartLba,
  IN  UINTN               NumberOfBlocks,
  IN  UINTN               BlockSize
  )
{

  ATAPI_PACKET_COMMAND  Packet;
  ATAPI_READ10_CMD      *Read10Packet;
  EFI_STATUS            Status;
  UINTN                 BlocksRemaining;
  UINT32                Lba32;
  UINT32                ByteCount;
  UINT16                SectorCount;
  VOID                  *PtrBuffer;
  UINT16                MaxBlock;

  //
  // fill command packet for Read(10) command
  //
  ZeroMem (&Packet, sizeof (ATAPI_PACKET_COMMAND));
  Read10Packet  = &Packet.Read10;
  Lba32         = (UINT32) StartLba;
  PtrBuffer     = Buffer;

  //
  // limit the data bytes that can be transfered by one Read(10) Command
  //
  MaxBlock = (UINT16) (0x10000 / BlockSize);
  //
  // (64k bytes)
  //
  BlocksRemaining = NumberOfBlocks;

  Status          = EFI_SUCCESS;
  while (BlocksRemaining > 0) {

    if (BlocksRemaining <= MaxBlock) {
      SectorCount = (UINT16) BlocksRemaining;
    } else {
      SectorCount = MaxBlock;
    }
    //
    // fill the Packet data sturcture
    //
    Read10Packet->opcode = ATA_CMD_READ_10;

    //
    // Lba0 ~ Lba3 specify the start logical block address of the data transfer.
    // Lba0 is MSB, Lba3 is LSB
    //
    Read10Packet->Lba3  = (UINT8) (Lba32 & 0xff);
    Read10Packet->Lba2  = (UINT8) (Lba32 >> 8);
    Read10Packet->Lba1  = (UINT8) (Lba32 >> 16);
    Read10Packet->Lba0  = (UINT8) (Lba32 >> 24);

    //
    // TranLen0 ~ TranLen1 specify the transfer length in block unit.
    // TranLen0 is MSB, TranLen is LSB
    //
    Read10Packet->TranLen1  = (UINT8) (SectorCount & 0xff);
    Read10Packet->TranLen0  = (UINT8) (SectorCount >> 8);

    ByteCount               = (UINT32) (SectorCount * BlockSize);

    Status = AtapiPacketCommandIn (
              AtapiBlkIoDev,
              DevicePosition,
              &Packet,
              (UINT16 *) PtrBuffer,
              ByteCount,
              ATAPILONGTIMEOUT
              );
    if (Status != EFI_SUCCESS) {
      return Status;
    }

    Lba32 += SectorCount;
    PtrBuffer = (UINT8 *) PtrBuffer + SectorCount * BlockSize;
    BlocksRemaining -= SectorCount;
  }

  return Status;
}

/**
  Check if there is media according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    No media
  @retval FALSE   Media exists

**/
BOOLEAN
IsNoMedia (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   IsNoMedia;

  IsNoMedia = FALSE;

  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    if ((SensePtr->sense_key == ATA_SK_NOT_READY) && (SensePtr->addnl_sense_code == ATA_ASC_NO_MEDIA)) {
      IsNoMedia = TRUE;
    }

    SensePtr++;
  }

  return IsNoMedia;
}

/**
  Check if device state is unclear according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    Device state is unclear
  @retval FALSE   Device state is clear

**/
BOOLEAN
IsDeviceStateUnclear (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                       SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   Unclear;

  Unclear  = FALSE;

  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    if (SensePtr->sense_key == 0x06) {
      //
      // Sense key is 0x06 means the device is just be reset or media just
      // changed. The current state of the device is unclear.
      //
      Unclear = TRUE;
      break;
    }

    SensePtr++;
  }

  return Unclear;
}

/**
  Check if there is media error according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.

  @retval TRUE    Media error
  @retval FALSE   No media error

**/
BOOLEAN
IsMediaError (
  IN  ATAPI_REQUEST_SENSE_DATA  *SenseData,
  IN  UINTN                     SenseCounts
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   IsError;

  IsError   = FALSE;

  SensePtr  = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->sense_key) {

    case ATA_SK_MEDIUM_ERROR:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_MEDIA_ERR1:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR2:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR3:
        //
        // fall through
        //
      case ATA_ASC_MEDIA_ERR4:
        IsError = TRUE;
        break;

      default:
        break;
      }

      break;

    case ATA_SK_NOT_READY:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_MEDIA_UPSIDE_DOWN:
        IsError = TRUE;
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return IsError;
}

/**
  Check if drive is ready according to sense data.

  @param[in]  SenseData   Pointer to sense data.
  @param[in]  SenseCounts Count of sense data.
  @param[out] NeedRetry   Indicate if retry is needed.

  @retval TRUE    Drive ready
  @retval FALSE   Drive not ready

**/
BOOLEAN
IsDriveReady (
  IN  ATAPI_REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                       SenseCounts,
  OUT BOOLEAN                     *NeedRetry
  )
{
  ATAPI_REQUEST_SENSE_DATA  *SensePtr;
  UINTN                     Index;
  BOOLEAN                   IsReady;

  IsReady     = TRUE;
  *NeedRetry  = FALSE;

  SensePtr    = SenseData;

  for (Index = 0; Index < SenseCounts; Index++) {

    switch (SensePtr->sense_key) {

    case ATA_SK_NOT_READY:
      switch (SensePtr->addnl_sense_code) {
      case ATA_ASC_NOT_READY:
        switch (SensePtr->addnl_sense_code_qualifier) {
        case ATA_ASCQ_IN_PROGRESS:
          IsReady     = FALSE;
          *NeedRetry  = TRUE;
          break;

        default:
          IsReady     = FALSE;
          *NeedRetry  = FALSE;
          break;
        }
        break;

      default:
        break;
      }
      break;

    default:
      break;
    }

    SensePtr++;
  }

  return IsReady;
}
