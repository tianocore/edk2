/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ide.c
    
Abstract: 
    

Revision History
--*/

#include "idebus.h"

BOOLEAN SlaveDeviceExist  = FALSE;
BOOLEAN MasterDeviceExist = FALSE;

UINT8
IDEReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description

Returns:

  TODO: add return values

--*/
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

VOID
IDEReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
/*++

Routine Description:
  Reads multiple words of data from the IDE data port. 
  Call the IO abstraction once to do the complete read,
  not one word at a time
  

Arguments:
  PciIo   - Pointer to the EFI_PCI_IO instance
  Port    - IO port to read
  Count   - No. of UINT16's to read
  Buffer  - Pointer to the data buffer for read

++*/
// TODO: function comment should end with '--*/'
// TODO: function comment is missing 'Returns:'
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

VOID
IDEWritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
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

VOID
IDEWritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
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

VOID
IDEWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
/*++

Routine Description:
  Write multiple words of data to the IDE data port. 
  Call the IO abstraction once to do the complete read,
  not one word at a time
  

Arguments:
  PciIo   - Pointer to the EFI_PCI_IO instance
  Port    - IO port to read
  Count   - No. of UINT16's to read
  Buffer  - Pointer to the data buffer for read

++*/
// TODO: function comment should end with '--*/'
// TODO: function comment is missing 'Returns:'
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

BOOLEAN
BadIdeDeviceCheck (
  IN IDE_BLK_IO_DEV *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  //
  //  check whether all registers return 0xff,
  //  if so, deem the channel is disabled.
  //
#ifdef EFI_DEBUG

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Data) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Head) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command) != 0xff) {
    return FALSE;
  }

  if (IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus) != 0xff) {
    return FALSE;
  }

  return TRUE;

#else

  return FALSE;

#endif
}

//
// GetIdeRegistersBaseAddr
//
EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  )
/*++

Routine Description:
  Get IDE IO port registers' base addresses by mode. In 'Compatibility' mode,
  use fixed addresses. In Native-PCI mode, get base addresses from BARs in
  the PCI IDE controller's Configuration Space.
  
  The steps to get IDE IO port registers' base addresses for each channel 
  as follows:

  1. Examine the Programming Interface byte of the Class Code fields in PCI IDE 
     controller's Configuration Space to determine the operating mode.
     
  2. a) In 'Compatibility' mode, use fixed addresses shown in the Table 1 below.
                 ___________________________________________
                |           | Command Block | Control Block |
                |  Channel  |   Registers   |   Registers   |
                |___________|_______________|_______________|
                |  Primary  |  1F0h - 1F7h  |  3F6h - 3F7h  |
                |___________|_______________|_______________|
                | Secondary |  170h - 177h  |  376h - 377h  |
                |___________|_______________|_______________|
        
                  Table 1. Compatibility resource mappings
        
     b) In Native-PCI mode, IDE registers are mapped into IO space using the BARs
        in IDE controller's PCI Configuration Space, shown in the Table 2 below.
               ___________________________________________________
              |           |   Command Block   |   Control Block   |
              |  Channel  |     Registers     |     Registers     |
              |___________|___________________|___________________|
              |  Primary  | BAR at offset 0x10| BAR at offset 0x14|
              |___________|___________________|___________________|
              | Secondary | BAR at offset 0x18| BAR at offset 0x1C|
              |___________|___________________|___________________|
      
                        Table 2. BARs for Register Mapping
        Note: Refer to Intel ICH4 datasheet, Control Block Offset: 03F4h for 
              primary, 0374h for secondary. So 2 bytes extra offset should be 
              added to the base addresses read from BARs.
  
  For more details, please refer to PCI IDE Controller Specification and Intel 
  ICH4 Datasheet.
  
Arguments:
  PciIo             - Pointer to the EFI_PCI_IO_PROTOCOL instance
  IdeRegsBaseAddr   - Pointer to IDE_REGISTERS_BASE_ADDR to 
                      receive IDE IO port registers' base addresses
                      
Returns:
    
--*/
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
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
    if ((PciData.Device.Bar[0] & bit0) == 0 || 
        (PciData.Device.Bar[1] & bit0) == 0) {
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
    if ((PciData.Device.Bar[2] & bit0) == 0 ||
        (PciData.Device.Bar[3] & bit0) == 0) {
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

EFI_STATUS
ReassignIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:
  This function is used to requery IDE resources. The IDE controller will 
  probably switch between native and legacy modes during the EFI->CSM->OS 
  transfer. We do this everytime before an BlkIo operation to ensure its
  succeess.
  
Arguments:
  IdeDev - The BLK_IO private data which specifies the IDE device
  
++*/
// TODO: function comment should end with '--*/'
// TODO: function comment is missing 'Returns:'
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS              Status;
  IDE_REGISTERS_BASE_ADDR IdeRegsBaseAddr[IdeMaxChannel];
  UINT16                  CommandBlockBaseAddr;
  UINT16                  ControlBlockBaseAddr;

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

EFI_STATUS
CheckPowerMode (
  IDE_BLK_IO_DEV    *IdeDev
  )
/*++
 Routine Description:
 
  Read SATA registers to detect SATA disks

 Arguments:

  IdeDev - The BLK_IO private data which specifies the IDE device
  
++*/
// TODO: function comment should end with '--*/'
// TODO: function comment is missing 'Returns:'
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  UINT8       ErrorRegister;
  EFI_STATUS  Status;

  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );

  //
  // Wait 31 seconds for BSY clear. BSY should be in clear state if there exists
  // a device (initial state). Normally, BSY is also in clear state if there is
  // no device
  //
  Status = WaitForBSYClear (IdeDev, 31000);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // select device, read error register
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );
  Status        = DRDYReady (IdeDev, 200);

  ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
  if ((ErrorRegister == 0x01) || (ErrorRegister == 0x81)) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

//
// DiscoverIdeDevice
//
EFI_STATUS
DiscoverIdeDevice (
  IN IDE_BLK_IO_DEV *IdeDev
  )
/*++
 Routine Description:
 
  Detect if there is disk connected to this port

 Arguments:

  IdeDev - The BLK_IO private data which specifies the IDE device
  
++*/
// TODO: function comment should end with '--*/'
// TODO: function comment is missing 'Returns:'
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS  Status;
  BOOLEAN     SataFlag;

  SataFlag = FALSE;
  //
  // This extra detection is for SATA disks
  //
  Status = CheckPowerMode (IdeDev);
  if (Status == EFI_SUCCESS) {
    SataFlag = TRUE;
  }
    
  //
  // If a channel has not been checked, check it now. Then set it to "checked" state
  // After this step, all devices in this channel have been checked.
  //
  Status = DetectIDEController (IdeDev);

  if ((EFI_ERROR (Status)) && !SataFlag) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Device exists. test if it is an ATA device
  //
  Status = ATAIdentify (IdeDev);
  if (EFI_ERROR (Status)) {
    //
    // if not ATA device, test if it is an ATAPI device
    //
    Status = ATAPIIdentify (IdeDev);
    if (EFI_ERROR (Status)) {
      //
      // if not ATAPI device either, return error.
      //
      return EFI_NOT_FOUND;
    }
  }

  //
  // Init Block I/O interface
  //
  IdeDev->BlkIo.Revision            = EFI_BLOCK_IO_PROTOCOL_REVISION;
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

EFI_STATUS
DetectIDEController (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++
  
  Name: DetectIDEController


  Purpose: 
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
 
            
  Parameters:
      IDE_BLK_IO_DEV  IN    *IdeDev
            pointer pointing to IDE_BLK_IO_DEV data structure, used
            to record all the information of the IDE device.


  Returns:    
      TRUE      
            successfully detects device.

      FALSE
            any failure during detection process will return this
            value.


  Notes:
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  EFI_STATUS  Status;
  UINT8       ErrorReg;
  UINT8       StatusReg;
  UINT8       InitStatusReg;
  EFI_STATUS  DeviceStatus;

  //
  // Slave device has been detected with master device.
  //
  if ((IdeDev->Device) == 1) {
    if (SlaveDeviceExist) {
      //
      // If master not exists but slave exists, slave have to wait a while
      //
      if (!MasterDeviceExist) {
        //
        // if single slave can't be detected, add delay 4s here.
        //
        gBS->Stall (4000000);
      }

      return EFI_SUCCESS;
    } else {
      return EFI_NOT_FOUND;
    }
  }
      
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
  // Select master back
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

  ErrorReg  = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);

  //
  // Master Error register is 0x01. D0 passed, D1 passed or not present.
  // Master Error register is 0x81. D0 passed, D1 failed. Return.
  // Master Error register is other value. D0 failed, D1 passed or not present..
  //
  if (ErrorReg == 0x01) {
    MasterDeviceExist = TRUE;
    DeviceStatus      = EFI_SUCCESS;
  } else if (ErrorReg == 0x81) {

    MasterDeviceExist = TRUE;
    DeviceStatus      = EFI_SUCCESS;
    SlaveDeviceExist  = FALSE;

    return DeviceStatus;
  } else {
    MasterDeviceExist = FALSE;
    DeviceStatus      = EFI_NOT_FOUND;
  }
    
  //
  // Master Error register is not 0x81, Go on check Slave
  //

  //
  // select slave
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((1 << 4) | 0xe0)
    );

  gBS->Stall (300);
  ErrorReg = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);

  //
  // Slave Error register is not 0x01, D1 failed. Return.
  //
  if (ErrorReg != 0x01) {
    SlaveDeviceExist = FALSE;
    return DeviceStatus;
  }

  StatusReg = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);

  //
  // Most ATAPI devices don't set DRDY bit, so test with a slow but accurate
  //   "ATAPI TEST UNIT READY" command
  //
  if (((StatusReg & DRDY) == 0) && ((InitStatusReg & DRDY) == 0)) {
    Status = AtapiTestUnitReady (IdeDev);

    //
    // Still fail, Slave doesn't exist.
    //
    if (EFI_ERROR (Status)) {
      SlaveDeviceExist = FALSE;
      return DeviceStatus;
    }
  }

  //
  // Error reg is 0x01 and DRDY is ready,
  //  or ATAPI test unit ready success,
  //  or  init Slave status DRDY is ready
  // Slave exists.
  //
  SlaveDeviceExist = TRUE;

  return DeviceStatus;

}

EFI_STATUS
DRQClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:   DRQClear


  Purpose: 
        This function is used to poll for the DRQ bit clear in the Status 
        Register. DRQ is cleared when the device is finished transferring data. 
        So this function is called after data transfer is finished.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
            pointer pointing to IDE_BLK_IO_DEV data structure, used
            to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
            used to designate the timeout for the DRQ clear.

  Returns:  
        EFI_SUCCESS
            DRQ bit clear within the time out.

        EFI_TIMEOUT
            DRQ bit not clear within the time out. 


  Notes:
        Read Status Register will clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
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
    if ((StatusRegister & (DRQ | BSY)) == 0) {
      break;
    }

    if ((StatusRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    //
    //  Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DRQClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:   DRQClear2


  Purpose: 
        This function is used to poll for the DRQ bit clear in the Alternate 
        Status Register. DRQ is cleared when the device is finished 
        transferring data. So this function is called after data transfer
        is finished.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ clear.

  Returns:  
        EFI_SUCCESS
          DRQ bit clear within the time out.

        EFI_TIMEOUT
          DRQ bit not clear within the time out. 


  Notes:
        Read Alternate Status Register will not clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
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
    if ((AltRegister & (DRQ | BSY)) == 0) {
      break;
    }

    if ((AltRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DRQReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:   DRQReady

  
  Purpose: 
        This function is used to poll for the DRQ bit set in the 
        Status Register.
        DRQ is set when the device is ready to transfer data. So this function
        is called after the command is sent to the device and before required 
        data is transferred.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
            pointer pointing to IDE_BLK_IO_DEV data structure,used
            to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
            used to designate the timeout for the DRQ ready.


  Returns:  
        EFI_SUCCESS
            DRQ bit set within the time out.

        EFI_TIMEOUT
            DRQ bit not set within the time out.
            
        EFI_ABORTED
            DRQ bit not set caused by the command abort.

  Notes:
        Read Status Register will clear interrupt status.

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
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
    if ((StatusRegister & (BSY | DRQ)) == DRQ) {
      break;
    }

    if ((StatusRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;
  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DRQReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:   DRQReady2


  Purpose: 
        This function is used to poll for the DRQ bit set in the 
        Alternate Status Register. DRQ is set when the device is ready to 
        transfer data. So this function is called after the command 
        is sent to the device and before required data is transferred.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ ready.

  Returns:  
        EFI_SUCCESS
          DRQ bit set within the time out.

        EFI_TIMEOUT
          DRQ bit not set within the time out. 
        
        EFI_ABORTED
            DRQ bit not set caused by the command abort.

  Notes:
        Read Alternate Status Register will not clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
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
    if ((AltRegister & (BSY | DRQ)) == DRQ) {
      break;
    }

    if ((AltRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;
  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
WaitForBSYClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:
        WaitForBSYClear


  Purpose: 
        This function is used to poll for the BSY bit clear in the 
        Status Register. BSY is clear when the device is not busy.
        Every command must be sent after device is not busy.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ ready.

  Returns:  
        EFI_SUCCESS
          BSY bit clear within the time out.

        EFI_TIMEOUT
          BSY bit not clear within the time out. 


  Notes:
        Read Status Register will clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
{
  UINT32  Delay;
  UINT8   StatusRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {

    StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);
    if ((StatusRegister & BSY) == 0x00) {
      break;
    }

    //
    // Stall for 30 us
    //
    gBS->Stall (30);

    Delay--;

  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}
//
// WaitForBSYClear2
//
EFI_STATUS
WaitForBSYClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++
  Name:
        WaitForBSYClear2


  Purpose: 
        This function is used to poll for the BSY bit clear in the 
        Alternate Status Register. BSY is clear when the device is not busy.
        Every command must be sent after device is not busy.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ ready.

  Returns:  
        EFI_SUCCESS
          BSY bit clear within the time out.

        EFI_TIMEOUT
          BSY bit not clear within the time out. 


  Notes:
        Read Alternate Status Register will not clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    TimeoutInMilliSeconds - add argument and description to function comment
{
  UINT32  Delay;
  UINT8   AltRegister;

  Delay = (UINT32) (((TimeoutInMilliSeconds * STALL_1_MILLI_SECOND) / 30) + 1);
  do {
    AltRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Alt.AltStatus);
    if ((AltRegister & BSY) == 0x00) {
      break;
    }

    gBS->Stall (30);

    Delay--;

  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

//
// DRDYReady
//
EFI_STATUS
DRDYReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
/*++
  Name:
        DRDYReady


  Purpose: 
        This function is used to poll for the DRDY bit set in the 
        Status Register. DRDY bit is set when the device is ready 
        to accept command. Most ATA commands must be sent after 
        DRDY set except the ATAPI Packet Command.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ ready.

  Returns:  
        EFI_SUCCESS
          DRDY bit set within the time out.

        EFI_TIMEOUT
          DRDY bit not set within the time out. 


  Notes:
        Read Status Register will clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    DelayInMilliSeconds - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
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
    if ((StatusRegister & (DRDY | BSY)) == DRDY) {
      break;
    }

    if ((StatusRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    gBS->Stall (15);

    Delay--;
  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

//
// DRDYReady2
//
EFI_STATUS
DRDYReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
/*++
  Name:
        DRDYReady2


  Purpose: 
        This function is used to poll for the DRDY bit set in the 
        Alternate Status Register. DRDY bit is set when the device is ready 
        to accept command. Most ATA commands must be sent after 
        DRDY set except the ATAPI Packet Command.


  Parameters:
        IDE_BLK_IO_DEV  IN    *IdeDev
          pointer pointing to IDE_BLK_IO_DEV data structure, used
          to record all the information of the IDE device.

        UINTN     IN    TimeoutInMilliSeconds
          used to designate the timeout for the DRQ ready.

  Returns:  
        EFI_SUCCESS
          DRDY bit set within the time out.

        EFI_TIMEOUT
          DRDY bit not set within the time out. 


  Notes:
        Read Alternate Status Register will clear interrupt status.
--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    IdeDev - add argument and description to function comment
// TODO:    DelayInMilliSeconds - add argument and description to function comment
// TODO:    EFI_ABORTED - add return value to function comment
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
    if ((AltRegister & (DRDY | BSY)) == DRDY) {
      break;
    }

    if ((AltRegister & (BSY | ERR)) == ERR) {

      ErrorRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Error);
      if ((ErrorRegister & ABRT_ERR) == ABRT_ERR) {
        return EFI_ABORTED;
      }
    }

    gBS->Stall (30);

    Delay--;
  } while (Delay);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

//
// SwapStringChars
//
VOID
SwapStringChars (
  IN CHAR8  *Destination,
  IN CHAR8  *Source,
  IN UINT32 Size
  )
/*++
  Name:
        SwapStringChars


  Purpose: 
        This function is a helper function used to change the char order in a 
        string. It is designed specially for the PrintAtaModuleName() function.
        After the IDE device is detected, the IDE driver gets the device module
        name by sending ATA command called ATA Identify Command or ATAPI 
        Identify Command to the specified IDE device. The module name returned 
        is a string of ASCII characters: the first character is bit8--bit15 
        of the first word, the second character is bit0--bit7 of the first word 
        and so on. Thus the string can not be print directly before it is 
        preprocessed by this func to change the order of characters in 
        each word in the string.


  Parameters:
        CHAR8 IN    *Destination
          Indicates the destination string.

        CHAR8 IN    *Source
          Indicates the source string.

        UINT8 IN    Size
          the length of the string


  Returns:  
        none

  Notes:

--*/
// TODO: function comment is missing 'Routine Description:'
// TODO: function comment is missing 'Arguments:'
// TODO:    Destination - add argument and description to function comment
// TODO:    Source - add argument and description to function comment
// TODO:    Size - add argument and description to function comment
{
  UINT32  Index;
  CHAR8   Temp;

  for (Index = 0; Index < Size; Index += 2) {

    Temp                    = Source[Index + 1];
    Destination[Index + 1]  = Source[Index];
    Destination[Index]      = Temp;
  }
}

//
// ReleaseIdeResources
//
VOID
ReleaseIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeBlkIoDevice
  )
/*++
Routing Description:

  Release resources of an IDE device before stopping it.

Arguments:

  IdeBlkIoDevice  --  Standard IDE device private data structure


Returns:

    NONE
    
---*/
// TODO: function comment is missing 'Routine Description:'
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

  if (IdeBlkIoDevice->pIdData != NULL) {
    gBS->FreePool (IdeBlkIoDevice->pIdData);
    IdeBlkIoDevice->pIdData = NULL;
  }

  if (IdeBlkIoDevice->pInquiryData != NULL) {
    gBS->FreePool (IdeBlkIoDevice->pInquiryData);
    IdeBlkIoDevice->pInquiryData = NULL;
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

  gBS->FreePool (IdeBlkIoDevice);
  IdeBlkIoDevice = NULL;

  return ;
}

//
// SetDeviceTransferMode
//
EFI_STATUS
SetDeviceTransferMode (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_TRANSFER_MODE    *TransferMode
  )
/*++
Routing Description:

  Set the calculated Best transfer mode to a detected device

Arguments:

  IdeDev       --  Standard IDE device private data structure
  TransferMode --  The device transfer mode to be set

Returns:

    Set transfer mode Command execute status
    
---*/
// TODO: function comment is missing 'Routine Description:'
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
            SET_FEATURES_CMD,
            DeviceSelect,
            0x03,
            SectorCount,
            0,
            0,
            0
            );

  return Status;
}

EFI_STATUS
AtaNonDataCommandIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT8           Feature,
  IN  UINT8           SectorCount,
  IN  UINT8           LbaLow,
  IN  UINT8           LbaMiddle,
  IN  UINT8           LbaHigh
  )
/*++

Routine Description:

  Send ATA command into device with NON_DATA protocol

Arguments:

  IdeDev      - Standard IDE device private data structure
  AtaCommand  - The ATA command to be sent
  Device      - The value in Device register
  Feature     - The value in Feature register
  SectorCount - The value in SectorCount register 
  LbaLow      - The value in LBA_LOW register
  LbaMiddle   - The value in LBA_MIDDLE register
  LbaHigh     - The value in LBA_HIGH register

Returns:

  EFI_SUCCESS      - Reading succeed
  EFI_ABORTED      - Command failed
  EFI_DEVICE_ERROR - Device status error

--*/
{
  EFI_STATUS  Status;
  UINT8       StatusRegister;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );

  //
  // ATA commands for ATA device must be issued when DRDY is set
  //
  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Pass parameter into device register block
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, Device);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMiddle);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  //
  // Send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  //
  // Wait for command completion
  //
  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);
  if ((StatusRegister & ERR) == ERR) {
    //
    // Failed to execute command, abort operation
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AtaNonDataCommandInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
/*++

Routine Description:

  Send ATA Ext command into device with NON_DATA protocol

Arguments:

  IdeDev      - Standard IDE device private data structure
  AtaCommand  - The ATA command to be sent
  Device      - The value in Device register
  Feature     - The value in Feature register
  SectorCount - The value in SectorCount register 
  LbaAddress - The LBA address in 48-bit mode

Returns:

  EFI_SUCCESS      - Reading succeed
  EFI_ABORTED      - Command failed
  EFI_DEVICE_ERROR - Device status error

--*/
{
  EFI_STATUS  Status;
  UINT8       StatusRegister;
  UINT8       SectorCount8;
  UINT8       Feature8;
  UINT8       LbaLow;
  UINT8       LbaMid;
  UINT8       LbaHigh;

  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Select device (bit4), set LBA mode(bit6) (use 0xe0 for compatibility)
  //
  IDEWritePortB (
    IdeDev->PciIo,
    IdeDev->IoPort->Head,
    (UINT8) ((IdeDev->Device << 4) | 0xe0)
    );

  //
  // ATA commands for ATA device must be issued when DRDY is set
  //
  Status = DRDYReady (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Pass parameter into device register block
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Head, Device);

  //
  // Fill the feature register, which is a two-byte FIFO. Need write twice.
  //
  Feature8 = (UINT8) (Feature >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature8);

  Feature8 = (UINT8) Feature;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg1.Feature, Feature8);

  //
  // Fill the sector count register, which is a two-byte FIFO. Need write twice.
  //
  SectorCount8 = (UINT8) (SectorCount >> 8);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  SectorCount8 = (UINT8) SectorCount;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorCount, SectorCount8);

  //
  // Fill the start LBA registers, which are also two-byte FIFO
  //
  LbaLow  = (UINT8) RShiftU64 (LbaAddress, 24);
  LbaMid  = (UINT8) RShiftU64 (LbaAddress, 32);
  LbaHigh = (UINT8) RShiftU64 (LbaAddress, 40);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  LbaLow  = (UINT8) LbaAddress;
  LbaMid  = (UINT8) RShiftU64 (LbaAddress, 8);
  LbaHigh = (UINT8) RShiftU64 (LbaAddress, 16);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->SectorNumber, LbaLow);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderLsb, LbaMid);
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->CylinderMsb, LbaHigh);

  //
  // Send command via Command Register
  //
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Command, AtaCommand);

  //
  // Wait for command completion
  //
  Status = WaitForBSYClear (IdeDev, ATATIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  StatusRegister = IDEReadPortB (IdeDev->PciIo, IdeDev->IoPort->Reg.Status);
  if ((StatusRegister & ERR) == ERR) {
    //
    // Failed to execute command, abort operation
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

//
// SetDriveParameters
//
EFI_STATUS
SetDriveParameters (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_DRIVE_PARMS      *DriveParameters
  )
/*++
Routine Description: 

  Set drive parameters for devices not support PACKETS command

Arguments:

  IdeDev       --  Standard IDE device private data structure
  DriveParameters --  The device parameters to be set into the disk

Returns:

  SetParameters Command execute status
    
--*/
{
  EFI_STATUS  Status;
  UINT8       DeviceSelect;

  DeviceSelect  = 0;
  DeviceSelect  = (UINT8) ((IdeDev->Device) << 4);

  //
  // Send Init drive parameters
  //
  Status = AtaPioDataIn (
            IdeDev,
            NULL,
            0,
            INIT_DRIVE_PARAM_CMD,
            (UINT8) (DeviceSelect + DriveParameters->Heads),
            DriveParameters->Sector,
            0,
            0,
            0
            );

  //
  // Send Set Multiple parameters
  //
  Status = AtaPioDataIn (
            IdeDev,
            NULL,
            0,
            SET_MULTIPLE_MODE_CMD,
            DeviceSelect,
            DriveParameters->MultipleSector,
            0,
            0,
            0
            );

  return Status;
}

EFI_STATUS
EnableInterrupt (
  IN IDE_BLK_IO_DEV       *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UINT8 DeviceControl;

  //
  // Enable interrupt for DMA operation
  //
  DeviceControl = 0;
  IDEWritePortB (IdeDev->PciIo, IdeDev->IoPort->Alt.DeviceControl, DeviceControl);

  return EFI_SUCCESS;
}
