/** @file
  The file ontaining the helper functions implement of the Ide Bus driver
  
  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IdeBus.h"

BOOLEAN ChannelDeviceDetected = FALSE;
BOOLEAN SlaveDeviceExist      = FALSE;
UINT8   SlaveDeviceType       = INVALID_DEVICE_TYPE;
BOOLEAN MasterDeviceExist     = FALSE;
UINT8   MasterDeviceType      = INVALID_DEVICE_TYPE;

/**
  read a one-byte data from a IDE port.

  @param  PciIo  The PCI IO protocol instance
  @param  Port   the IDE Port number 

  @return  the one-byte data read from IDE port
**/
UINT8
IDEReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
{
  UINT8 Data;

  Data = 0;
  //
  // perform 1-byte data read from register
  //
  PciIo->Io.Read (
              PciIo,
              EfiPciIoWidthUint8,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64) Port,
              1,
              &Data
              );
  return Data;
}
/**
  Reads multiple words of data from the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo Pointer to the EFI_PCI_IO instance
  @param  Port IO port to read
  @param  Count No. of UINT16's to read
  @param  Buffer Pointer to the data buffer for read

**/
VOID
IDEReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  OUT VOID                  *Buffer
  )
{
  UINT16  *AlignedBuffer;
  UINT16  *WorkingBuffer;
  UINTN   Size;

  //
  // Prepare an 16-bit alligned working buffer. CpuIo will return failure and
  // not perform actual I/O operations if buffer pointer passed in is not at
  // natural boundary. The "Buffer" argument is passed in by user and may not
  // at 16-bit natural boundary.
  //
  Size = sizeof (UINT16) * Count;

  gBS->AllocatePool (
        EfiBootServicesData,
        Size + 1,
        (VOID**)&WorkingBuffer
        );

  AlignedBuffer = (UINT16 *) ((UINTN)(((UINTN) WorkingBuffer + 0x1) & (~0x1)));

  //
  // Perform UINT16 data read from FIFO
  //
  PciIo->Io.Read (
              PciIo,
              EfiPciIoWidthFifoUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64) Port,
              Count,
              (UINT16*)AlignedBuffer
              );

  //
  // Copy data to user buffer
  //
  CopyMem (Buffer, (UINT16*)AlignedBuffer, Size);
  gBS->FreePool (WorkingBuffer);
}

/**
  write a 1-byte data to a specific IDE port.

  @param  PciIo  PCI IO protocol instance
  @param  Port   The IDE port to be writen
  @param  Data   The data to write to the port
**/
VOID
IDEWritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  )
{
  //
  // perform 1-byte data write to register
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint8,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64) Port,
              1,
              &Data
              );

}

/**
  write a 1-word data to a specific IDE port.

  @param  PciIo  PCI IO protocol instance
  @param  Port   The IDE port to be writen
  @param  Data   The data to write to the port
**/
VOID
IDEWritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  )
{
  //
  // perform 1-word data write to register
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64) Port,
              1,
              &Data
              );
}

/**
  Write multiple words of data to the IDE data port.
  Call the IO abstraction once to do the complete read,
  not one word at a time

  @param  PciIo Pointer to the EFI_PCI_IO instance
  @param  Port IO port to read
  @param  Count No. of UINT16's to read
  @param  Buffer Pointer to the data buffer for read

**/
VOID
IDEWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
{
  UINT16  *AlignedBuffer;
  UINT32  *WorkingBuffer;
  UINTN   Size;

  //
  // Prepare an 16-bit alligned working buffer. CpuIo will return failure and
  // not perform actual I/O operations if buffer pointer passed in is not at
  // natural boundary. The "Buffer" argument is passed in by user and may not
  // at 16-bit natural boundary.
  //
  Size = sizeof (UINT16) * Count;

  gBS->AllocatePool (
        EfiBootServicesData,
        Size + 1,
        (VOID **) &WorkingBuffer
        );

  AlignedBuffer = (UINT16 *) ((UINTN)(((UINTN) WorkingBuffer + 0x1) & (~0x1)));

  //
  // Copy data from user buffer to working buffer
  //
  CopyMem ((UINT16 *) AlignedBuffer, Buffer, Size);

  //
  // perform UINT16 data write to the FIFO
  //
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthFifoUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              (UINT64) Port,
              Count,
              (UINT16 *) AlignedBuffer
              );

  gBS->FreePool (WorkingBuffer);
}
/**
  Get IDE IO port registers' base addresses by mode. In 'Compatibility' mode,
  use fixed addresses. In Native-PCI mode, get base addresses from BARs in
  the PCI IDE controller's Configuration Space.

  The steps to get IDE IO port registers' base addresses for each channel
  as follows:

  1. Examine the Programming Interface byte of the Class Code fields in PCI IDE
  controller's Configuration Space to determine the operating mode.

  2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
  <pre>
  ___________________________________________
  |           | Command Block | Control Block |
  |  Channel  |   Registers   |   Registers   |
  |___________|_______________|_______________|
  |  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
  |___________|_______________|_______________|
  | Secondary |  170h - 177h  |  376h - 377h  |
  |___________|_______________|_______________|

  Table 1. Compatibility resource mappings
  </pre>

  b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
  in IDE controller's PCI Configuration Space, shown in the Table 2 below.
  <pre>
  ___________________________________________________
  |           |   Command Block   |   Control Block   |
  |  Channel  |     Registers     |     Registers     |
  |___________|___________________|___________________|
  |  Primary  | BAR at offset 0x10| BAR at offset 0x14|
  |___________|___________________|___________________|
  | Secondary | BAR at offset 0x18| BAR at offset 0x1C|
  |___________|___________________|___________________|

  Table 2. BARs for Register Mapping
  </pre>
  @note Refer to Intel ICH4 datasheet, Control Block Offset: 03F4h for
  primary, 0374h for secondary. So 2 bytes extra offset should be
  added to the base addresses read from BARs.

  For more details, please refer to PCI IDE Controller Specification and Intel
  ICH4 Datasheet.

  @param  PciIo Pointer to the EFI_PCI_IO_PROTOCOL instance
  @param  IdeRegsBaseAddr Pointer to IDE_REGISTERS_BASE_ADDR to
           receive IDE IO port registers' base addresses
           
  @retval EFI_UNSUPPORTED return this value when the BARs is not IO type
  @retval EFI_SUCCESS     Get the Base address successfully
  @retval other           read the pci configureation data error

**/
EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  )
{
  EFI_STATUS  Status;
  PCI_TYPE00  PciData;

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PciData),
                        &PciData
                        );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((PciData.Hdr.ClassCode[0] & IDE_PRIMARY_OPERATING_MODE) == 0) {
    IdeRegsBaseAddr[IdePrimary].CommandBlockBaseAddr  = 0x1f0;
    IdeRegsBaseAddr[IdePrimary].ControlBlockBaseAddr  = 0x3f6;
    IdeRegsBaseAddr[IdePrimary].BusMasterBaseAddr     =
    (UINT16)((PciData.Device.Bar[4] & 0x0000fff0));
  } else {
    //
    // The BARs should be of IO type
    //
    if ((PciData.Device.Bar[0] & BIT0) == 0 ||
        (PciData.Device.Bar[1] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    IdeRegsBaseAddr[IdePrimary].CommandBlockBaseAddr  =
    (UINT16) (PciData.Device.Bar[0] & 0x0000fff8);
    IdeRegsBaseAddr[IdePrimary].ControlBlockBaseAddr  =
    (UINT16) ((PciData.Device.Bar[1] & 0x0000fffc) + 2);
    IdeRegsBaseAddr[IdePrimary].BusMasterBaseAddr     =
    (UINT16) ((PciData.Device.Bar[4] & 0x0000fff0));
  }

  if ((PciData.Hdr.ClassCode[0] & IDE_SECONDARY_OPERATING_MODE) == 0) {
    IdeRegsBaseAddr[IdeSecondary].CommandBlockBaseAddr  = 0x170;
    IdeRegsBaseAddr[IdeSecondary].ControlBlockBaseAddr  = 0x376;
    IdeRegsBaseAddr[IdeSecondary].BusMasterBaseAddr     =
    (UINT16) ((PciData.Device.Bar[4] & 0x0000fff0));
  } else {
    //
    // The BARs should be of IO type
    //
    if ((PciData.Device.Bar[2] & BIT0) == 0 ||
        (PciData.Device.Bar[3] & BIT0) == 0) {
      return EFI_UNSUPPORTED;
    }

    IdeRegsBaseAddr[IdeSecondary].CommandBlockBaseAddr  =
    (UINT16) (PciData.Device.Bar[2] & 0x0000fff8);
    IdeRegsBaseAddr[IdeSecondary].ControlBlockBaseAddr  =
    (UINT16) ((PciData.Device.Bar[3] & 0x0000fffc) + 2);
    IdeRegsBaseAddr[IdeSecondary].BusMasterBaseAddr     =
    (UINT16) ((PciData.Device.Bar[4] & 0x0000fff0));
  }

  return EFI_SUCCESS;
}

/**
  This function is used to requery IDE resources. The IDE controller will
  probably switch between native and legacy modes during the EFI->CSM->OS
  transfer. We do this everytime before an BlkIo operation to ensure its
  succeess.

  @param  IdeDev The BLK_IO private data which specifies the IDE device
  
  @retval EFI_INVALID_PARAMETER return this value when the channel is invalid
  @retval EFI_SUCCESS           reassign the IDE IO resource successfully
  @retval other                 get the IDE current base address effor

**/
EFI_STATUS
ReassignIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  EFI_STATUS              Status;
  IDE_REGISTERS_BASE_ADDR IdeRegsBaseAddr[IdeMaxChannel];
  UINT16                  CommandBlockBaseAddr;
  UINT16                  ControlBlockBaseAddr;

  if (IdeDev->Channel >= IdeMaxChannel) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Requery IDE IO port registers' base addresses in case of the switch of
  // native and legacy modes
  //
  Status = GetIdeRegistersBaseAddr (IdeDev->PciIo, IdeRegsBaseAddr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (IdeDev->IoPort, sizeof (IDE_BASE_REGISTERS));
  CommandBlockBaseAddr                = IdeRegsBaseAddr[IdeDev->Channel].CommandBlockBaseAddr;
  ControlBlockBaseAddr                = IdeRegsBaseAddr[IdeDev->Channel].ControlBlockBaseAddr;

  IdeDev->IoPort->Data                = CommandBlockBaseAddr;
  (*(UINT16 *) &IdeDev->IoPort->Reg1) = (UINT16) (CommandBlockBaseAddr + 0x01);
  IdeDev->IoPort->SectorCount         = (UINT16) (CommandBlockBaseAddr + 0x02);
  IdeDev->IoPort->SectorNumber        = (UINT16) (CommandBlockBaseAddr + 0x03);
  IdeDev->IoPort->CylinderLsb         = (UINT16) (CommandBlockBaseAddr + 0x04);
  IdeDev->IoPort->CylinderMsb         = (UINT16) (CommandBlockBaseAddr + 0x05);
  IdeDev->IoPort->Head                = (UINT16) (CommandBlockBaseAddr + 0x06);

  (*(UINT16 *) &IdeDev->IoPort->Reg)  = (UINT16) (CommandBlockBaseAddr + 0x07);
  (*(UINT16 *) &IdeDev->IoPort->Alt)  = ControlBlockBaseAddr;
  IdeDev->IoPort->DriveAddress        = (UINT16) (ControlBlockBaseAddr + 0x01);
  IdeDev->IoPort->MasterSlave         = (UINT16) ((IdeDev->Device == IdeMaster) ? 1 : 0);

  IdeDev->IoPort->BusMasterBaseAddr   = IdeRegsBaseAddr[IdeDev->Channel].BusMasterBaseAddr;
  return EFI_SUCCESS;
}

/**
  This function is called by DiscoverIdeDevice(). It is used for detect
  whether the IDE device exists in the specified Channel as the specified
  Device Number.

  There is two IDE channels: one is Primary Channel, the other is
  Secondary Channel.(Channel is the logical name for the physical "Cable".)
  Different channel has different register group.

  On each IDE channel, at most two IDE devices attach,
  one is called Device 0 (Master device), the other is called Device 1
  (Slave device). The devices on the same channel co-use the same register
  group, so before sending out a command for a specified device via command
  register, it is a must to select the current device to accept the command
  by set the device number in the Head/Device Register.

  @param IdeDev  pointer to IDE_BLK_IO_DEV data structure, used to record all the
                 information of the IDE device.

  @retval EFI_SUCCESS successfully detects device.

  @retval other       any failure during detection process will return this value.

**/
EFI_STATUS
DetectIDEController (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
{
  EFI_STATUS  Status;
  UINT8       SectorCountReg;
  UINT8       LBALowReg;
  UINT8       LBAMidReg;
  UINT8       LBAHighReg;
  UINT8       InitStatusReg;
  UINT8       StatusReg;

  //
  // Select slave device
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((1 << 4) | 0xe0)
    );
  gBS->Stall (100);

  //
  // Save the init slave status register
  //
  InitStatusReg = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

  //
  // Select Master back
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((0 << 4) | 0xe0)
    );
  gBS->Stall (100);

  //
  // Send ATA Device Execut Diagnostic command.
  // This command should work no matter DRDY is ready or not
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, 0x90);

  Status    = WaitForBSYClear (IdeDev, 3500);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "New detecting method: Send Execute Diagnostic Command: WaitForBSYClear: Status: %d\n", Status));
    return Status;
  }
  //
  // Read device signature
  //
  //
  // Select Master
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((0 << 4) | 0xe0)
    );
  gBS->Stall (100);
  SectorCountReg = IDEReadPortB (
                     IdeDev->PciIo,
                     IdeDev->IoPort->SectorCount
                     );
  LBALowReg      = IDEReadPortB (
                     IdeDev->PciIo,
                     IdeDev->IoPort->SectorNumber
                     );
  LBAMidReg      = IDEReadPortB (
                     IdeDev->PciIo,
                     IdeDev->IoPort->CylinderLsb
                     );
  LBAHighReg     = IDEReadPortB (
                     IdeDev->PciIo,
                     IdeDev->IoPort->CylinderMsb
                     );
  if ((SectorCountReg == 0x1) &&
      (LBALowReg      == 0x1) &&
      (LBAMidReg      == 0x0) &&
      (LBAHighReg     == 0x0)) {
    MasterDeviceExist = TRUE;
    MasterDeviceType  = ATA_DEVICE_TYPE;
  } else {
    if ((LBAMidReg      == 0x14) &&
        (LBAHighReg     == 0xeb)) {
      MasterDeviceExist = TRUE;
      MasterDeviceType  = ATAPI_DEVICE_TYPE;
    }
  }

  //
  // For some Hard Drive, it takes some time to get
  // the right signature when operating in single slave mode.
  // We stall 20ms to work around this.
  //
  if (!MasterDeviceExist) {
    gBS->Stall (20000);
  }

  //
  // Select Slave
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((1 << 4) | 0xe0)
    );
  gBS->Stall (100);
  SectorCountReg = IDEReadPortB (
                     IdeDev->PciIo,
                     IdeDev->IoPort->SectorCount
                     );
  LBALowReg  = IDEReadPortB (
                 IdeDev->PciIo,
                 IdeDev->IoPort->SectorNumber
                 );
  LBAMidReg  = IDEReadPortB (
                 IdeDev->PciIo,
                 IdeDev->IoPort->CylinderLsb
                 );
  LBAHighReg = IDEReadPortB (
                 IdeDev->PciIo,
                 IdeDev->IoPort->CylinderMsb
                 );
  StatusReg  = IDEReadPortB (
                 IdeDev->PciIo,
                 IdeDev->IoPort->Reg.Status
                 );
  if ((SectorCountReg == 0x1) &&
      (LBALowReg      == 0x1) &&
      (LBAMidReg      == 0x0) &&
      (LBAHighReg     == 0x0)) {
    SlaveDeviceExist = TRUE;
    SlaveDeviceType  = ATA_DEVICE_TYPE;
  } else {
    if ((LBAMidReg     == 0x14) &&
        (LBAHighReg    == 0xeb)) {
      SlaveDeviceExist = TRUE;
      SlaveDeviceType  = ATAPI_DEVICE_TYPE;
    }
  }

  //
  // When single master is plugged, slave device
  // will be wrongly detected. Here's the workaround
  // for ATA devices by detecting DRY bit in status
  // register.
  // NOTE: This workaround doesn't apply to ATAPI.
  //
  if (MasterDeviceExist && SlaveDeviceExist &&
      (StatusReg & ATA_STSREG_DRDY) == 0               &&
      (InitStatusReg & ATA_STSREG_DRDY) == 0           &&
      MasterDeviceType == SlaveDeviceType   &&
      SlaveDeviceType != ATAPI_DEVICE_TYPE) {
    SlaveDeviceExist = FALSE;
  }

  //
  // Indicate this channel has been detected
  //
  ChannelDeviceDetected = TRUE;
  return EFI_SUCCESS;
}
/**
  Detect if there is disk attached to this port

  @param  IdeDev The BLK_IO private data which specifies the IDE device.
  
  @retval EFI_NOT_FOUND   The device or channel is not found
  @retval EFI_SUCCESS     The device is found

**/
EFI_STATUS
DiscoverIdeDevice (
  IN IDE_BLK_IO_DEV *IdeDev
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  LongPhyStatus;

  //
  // If a channel has not been checked, check it now. Then set it to "checked" state
  // After this step, all devices in this channel have been checked.
  //
  if (!ChannelDeviceDetected) {
    Status = DetectIDEController (IdeDev);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  }

  Status = EFI_NOT_FOUND;

  //
  // Device exists. test if it is an ATA device.
  // Prefer the result from DetectIDEController,
  // if failed, try another device type to handle
  // devices that not follow the spec.
  //
  if ((IdeDev->Device == IdeMaster) && (MasterDeviceExist)) {
    if (MasterDeviceType == ATA_DEVICE_TYPE) {
      Status = ATAIdentify (IdeDev);
      if (EFI_ERROR (Status)) {
        Status = ATAPIIdentify (IdeDev);
        if (!EFI_ERROR (Status)) {
          MasterDeviceType = ATAPI_DEVICE_TYPE;
        }
      }
    } else {
      Status = ATAPIIdentify (IdeDev);
      if (EFI_ERROR (Status)) {
        Status = ATAIdentify (IdeDev);
        if (!EFI_ERROR (Status)) {
          MasterDeviceType = ATA_DEVICE_TYPE;
        }
      }
    }
  }
  if ((IdeDev->Device == IdeSlave) && (SlaveDeviceExist)) {
    if (SlaveDeviceType == ATA_DEVICE_TYPE) {
      Status = ATAIdentify (IdeDev);
      if (EFI_ERROR (Status)) {
        Status = ATAPIIdentify (IdeDev);
        if (!EFI_ERROR (Status)) {
          SlaveDeviceType = ATAPI_DEVICE_TYPE;
        }
      }
    } else {
      Status = ATAPIIdentify (IdeDev);
      if (EFI_ERROR (Status)) {
        Status = ATAIdentify (IdeDev);
        if (!EFI_ERROR (Status)) {
          SlaveDeviceType = ATA_DEVICE_TYPE;
        }
      }
    }
  }
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  //
  // Init Block I/O interface
  //
  LongPhyStatus = AtaEnableLongPhysicalSector (IdeDev);
  if (!EFI_ERROR (LongPhyStatus)) {
    IdeDev->BlkIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION2;
  } else {
    IdeDev->BlkIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION;
  }
  IdeDev->BlkIo.Reset               = IDEBlkIoReset;
  IdeDev->BlkIo.ReadBlocks          = IDEBlkIoReadBlocks;
  IdeDev->BlkIo.WriteBlocks         = IDEBlkIoWriteBlocks;
  IdeDev->BlkIo.FlushBlocks         = IDEBlkIoFlushBlocks;

  IdeDev->BlkMedia.LogicalPartition = FALSE;
  IdeDev->BlkMedia.WriteCaching     = FALSE;

  //
  // Init Disk Info interface
  //
  gBS->CopyMem (&IdeDev->DiskInfo.Interface, &gEfiDiskInfoIdeInterfaceGuid, sizeof (EFI_GUID));
  IdeDev->DiskInfo.Inquiry    = IDEDiskInfoInquiry;
  IdeDev->DiskInfo.Identify   = IDEDiskInfoIdentify;
  IdeDev->DiskInfo.SenseData  = IDEDiskInfoSenseData;
  IdeDev->DiskInfo.WhichIde   = IDEDiskInfoWhichIde;

  return EFI_SUCCESS;
}

/**
  This interface is used to initialize all state data related to the detection of one
  channel.
**/
VOID
InitializeIDEChannelData (
  VOID
  )
{
  ChannelDeviceDetected = FALSE;
  MasterDeviceExist = FALSE;
  MasterDeviceType  = 0xff;
  SlaveDeviceExist  = FALSE;
  SlaveDeviceType   = 0xff;
}
/**
  This function is used to poll for the DRQ bit clear in the Status
  Register. DRQ is cleared when the device is finished transferring data.
  So this function is called after data transfer is finished.

  @param IdeDev                 pointer pointing to IDE_BLK_IO_DEV data structure, used 
                                to record all the information of the IDE device.
  @param TimeoutInMilliSeconds  used to designate the timeout for the DRQ clear.

  @retval EFI_SUCCESS           DRQ bit clear within the time out.

  @retval EFI_TIMEOUT           DRQ bit not clear within the time out.

  @note
  Read Status Register will clear interrupt status.

**/
EFI_STATUS
DRQClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {

    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

    //
    // wait for BSY == 0 and DRQ == 0
    //
    if ((StatusRegister & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

    if ((StatusRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    //  Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the DRQ bit clear in the Alternate
  Status Register. DRQ is cleared when the device is finished
  transferring data. So this function is called after data transfer
  is finished.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used 
                               to record all the information of the IDE device.

  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ clear.

  @retval EFI_SUCCESS          DRQ bit clear within the time out.

  @retval EFI_TIMEOUT          DRQ bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
DRQClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {

    AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);

    //
    //  wait for BSY == 0 and DRQ == 0
    //
    if ((AltRegister & (ATA_STSREG_DRQ | ATA_STSREG_BSY)) == 0) {
      break;
    }

    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  This function is used to poll for the DRQ bit set in the
  Status Register.
  DRQ is set when the device is ready to transfer data. So this function
  is called after the command is sent to the device and before required
  data is transferred.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure,used to
                               record all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS          DRQ bit set within the time out.
  @retval EFI_TIMEOUT          DRQ bit not set within the time out.
  @retval EFI_ABORTED          DRQ bit not set caused by the command abort.

  @note  Read Status Register will clear interrupt status.

**/
EFI_STATUS
DRQReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {
    //
    //  read Status Register will clear interrupt
    //
    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

    //
    //  BSY==0,DRQ==1
    //
    if ((StatusRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((StatusRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the DRQ bit set in the Alternate Status Register.
  DRQ is set when the device is ready to transfer data. So this function is called after 
  the command is sent to the device and before required data is transferred.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used to 
                               record all the information of the IDE device.

  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS           DRQ bit set within the time out.
  @retval EFI_TIMEOUT           DRQ bit not set within the time out.
  @retval EFI_ABORTED           DRQ bit not set caused by the command abort.
  @note  Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
DRQReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);

  do {
    //
    //  Read Alternate Status Register will not clear interrupt status
    //
    AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);
    //
    // BSY == 0 , DRQ == 1
    //
    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_DRQ)) == ATA_STSREG_DRQ) {
      break;
    }

    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  This function is used to poll for the BSY bit clear in the Status Register. BSY
  is clear when the device is not busy. Every command must be sent after device is not busy.

  @param IdeDev                pointer pointing to IDE_BLK_IO_DEV data structure, used 
                               to record all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS          BSY bit clear within the time out.
  @retval EFI_TIMEOUT          BSY bit not clear within the time out.

  @note Read Status Register will clear interrupt status.
**/
EFI_STATUS
WaitForBSYClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {

    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);
    if ((StatusRegister & ATA_STSREG_BSY) == 0x00) {
      break;
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the BSY bit clear in the Alternate Status Register. 
  BSY is clear when the device is not busy. Every command must be sent after device is 
  not busy.

  @param IdeDev               pointer pointing to IDE_BLK_IO_DEV data structure, used to record 
                              all the information of the IDE device.
  @param TimeoutInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS         BSY bit clear within the time out.
  @retval EFI_TIMEOUT         BSY bit not clear within the time out.
  @note   Read Alternate Status Register will not clear interrupt status.

**/
EFI_STATUS
WaitForBSYClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   AltRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {
    AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);
    if ((AltRegister & ATA_STSREG_BSY) == 0x00) {
      break;
    }

    gBS->Stall (30);

    Delay--;

  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the DRDY bit set in the Status Register. DRDY
  bit is set when the device is ready to accept command. Most ATA commands must be 
  sent after DRDY set except the ATAPI Packet Command.

  @param IdeDev               pointer pointing to IDE_BLK_IO_DEV data structure, used
                              to record all the information of the IDE device.
  @param DelayInMilliSeconds  used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS         DRDY bit set within the time out.
  @retval EFI_TIMEOUT         DRDY bit not set within the time out.

  @note  Read Status Register will clear interrupt status.
**/
EFI_STATUS
DRDYReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   StatusRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((DelayInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {
    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);
    //
    //  BSY == 0 , DRDY == 1
    //
    if ((StatusRegister & (ATA_STSREG_DRDY | ATA_STSREG_BSY)) == ATA_STSREG_DRDY) {
      break;
    }

    if ((StatusRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    gBS->Stall (30);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  This function is used to poll for the DRDY bit set in the Alternate Status Register. 
  DRDY bit is set when the device is ready to accept command. Most ATA commands must 
  be sent after DRDY set except the ATAPI Packet Command.

  @param IdeDev              pointer pointing to IDE_BLK_IO_DEV data structure, used
                             to record all the information of the IDE device.
  @param DelayInMilliSeconds used to designate the timeout for the DRQ ready.

  @retval EFI_SUCCESS      DRDY bit set within the time out.
  @retval EFI_TIMEOUT      DRDY bit not set within the time out.

  @note  Read Alternate Status Register will clear interrupt status.

**/
EFI_STATUS
DRDYReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
{
  UINT32  Delay;
  UINT8   AltRegister;
  UINT8   ErrorRegister;

  Delay = (UINT32) (((DelayInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {
    AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);
    //
    //  BSY == 0 , DRDY == 1
    //
    if ((AltRegister & (ATA_STSREG_DRDY | ATA_STSREG_BSY)) == ATA_STSREG_DRDY) {
      break;
    }

    if ((AltRegister & (ATA_STSREG_BSY | ATA_STSREG_ERR)) == ATA_STSREG_ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ATA_ERRREG_ABRT) == ATA_ERRREG_ABRT) {
        return EFI_ABORTED;
      }
    }

    gBS->Stall (30);

    Delay--;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
/**
  Release resources of an IDE device before stopping it.

  @param IdeBlkIoDevice  Standard IDE device private data structure

**/
VOID
ReleaseIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeBlkIoDevice
  )
{
  if (IdeBlkIoDevice == NULL) {
    return ;
  }

  //
  // Release all the resourses occupied by the IDE_BLK_IO_DEV
  //

  if (IdeBlkIoDevice->SenseData != NULL) {
    gBS->FreePool (IdeBlkIoDevice->SenseData);
    IdeBlkIoDevice->SenseData = NULL;
  }

  if (IdeBlkIoDevice->Cache != NULL) {
    gBS->FreePool (IdeBlkIoDevice->Cache);
    IdeBlkIoDevice->Cache = NULL;
  }

  if (IdeBlkIoDevice->IdData != NULL) {
    gBS->FreePool (IdeBlkIoDevice->IdData);
    IdeBlkIoDevice->IdData = NULL;
  }

  if (IdeBlkIoDevice->InquiryData != NULL) {
    gBS->FreePool (IdeBlkIoDevice->InquiryData);
    IdeBlkIoDevice->InquiryData = NULL;
  }

  if (IdeBlkIoDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (IdeBlkIoDevice->ControllerNameTable);
    IdeBlkIoDevice->ControllerNameTable = NULL;
  }

  if (IdeBlkIoDevice->IoPort != NULL) {
    gBS->FreePool (IdeBlkIoDevice->IoPort);
  }

  if (IdeBlkIoDevice->DevicePath != NULL) {
    gBS->FreePool (IdeBlkIoDevice->DevicePath);
  }

  if (IdeBlkIoDevice->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (IdeBlkIoDevice->ExitBootServiceEvent);
    IdeBlkIoDevice->ExitBootServiceEvent = NULL;
  }

  gBS->FreePool (IdeBlkIoDevice);
  IdeBlkIoDevice = NULL;

  return ;
}
/**
  Set the calculated Best transfer mode to a detected device.

  @param IdeDev       Standard IDE device private data structure
  @param TransferMode The device transfer mode to be set
  @return Set transfer mode Command execute status.

**/
EFI_STATUS
SetDeviceTransferMode (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_TRANSFER_MODE    *TransferMode
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceSelect;
  UINT8       SectorCount;

  DeviceSelect  = 0;
  DeviceSelect  = (UINT8) ((IdeDev->Device) << 4);
  SectorCount   = *((UINT8 *) TransferMode);

  //
  // Send SET FEATURE command (sub command 0x03) to set pio mode.
  //
  Status = AtaNonDataCommandIn (
            IdeDev,
            ATA_CMD_SET_FEATURES,
            DeviceSelect,
            0x03,
            SectorCount,
            0,
            0,
            0
            );

  return Status;
}
/**
  Set drive parameters for devices not support PACKETS command.

  @param IdeDev          Standard IDE device private data structure
  @param DriveParameters The device parameters to be set into the disk
  @return SetParameters Command execute status.

**/
EFI_STATUS
SetDriveParameters (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_DRIVE_PARMS      *DriveParameters
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceSelect;

  DeviceSelect  = 0;
  DeviceSelect  = (UINT8) ((IdeDev->Device) << 4);

  //
  // Send Init drive parameters
  //
  Status = AtaNonDataCommandIn (
            IdeDev,
            ATA_CMD_INIT_DRIVE_PARAM,
            (UINT8) (DeviceSelect + DriveParameters->Heads),
            0,
            DriveParameters->Sector,
            0,
            0,
            0
            );

  //
  // Send Set Multiple parameters
  //
  Status = AtaNonDataCommandIn (
            IdeDev,
            ATA_CMD_SET_MULTIPLE_MODE,
            DeviceSelect,
            0,
            DriveParameters->MultipleSector,
            0,
            0,
            0
            );
  return Status;
}

/**
  Enable Interrupt on IDE controller.

  @param  IdeDev   Standard IDE device private data structure

  @retval  EFI_SUCCESS Enable Interrupt successfully
**/
EFI_STATUS
EnableInterrupt (
  IN IDE_BLK_IO_DEV       *IdeDev
  )
{
  UINT8 DeviceControl;

  //
  // Enable interrupt for DMA operation
  //
  DeviceControl = 0;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  return EFI_SUCCESS;
}
