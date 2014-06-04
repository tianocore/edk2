/** @file
  Internal floppy disk controller programming functions for the floppy driver.
  
Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IsaFloppy.h"

/**
  Detect whether a floppy drive is present or not.
 
  @param[in] FdcDev  A pointer to the FDC_BLK_IO_DEV

  @retval EFI_SUCCESS    The floppy disk drive is present
  @retval EFI_NOT_FOUND  The floppy disk drive is not present
**/
EFI_STATUS
DiscoverFddDevice (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;

  FdcDev->BlkIo.Media = &FdcDev->BlkMedia;

  Status = FddIdentify (FdcDev);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  FdcDev->BlkIo.Reset               = FdcReset;
  FdcDev->BlkIo.FlushBlocks         = FddFlushBlocks;
  FdcDev->BlkIo.ReadBlocks          = FddReadBlocks;
  FdcDev->BlkIo.WriteBlocks         = FddWriteBlocks;
  FdcDev->BlkMedia.LogicalPartition = FALSE;
  FdcDev->BlkMedia.WriteCaching     = FALSE;

  return EFI_SUCCESS;
}

/**
  Do recalibrate and check if the drive is present or not
  and set the media parameters if the driver is present.
  
  @param[in] FdcDev  A pointer to the FDC_BLK_IO_DEV

  @retval EFI_SUCCESS       The floppy disk drive is present
  @retval EFI_DEVICE_ERROR  The floppy disk drive is not present
**/
EFI_STATUS
FddIdentify (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;

  //
  // Set Floppy Disk Controller's motor on
  //
  Status = MotorOn (FdcDev);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = Recalibrate (FdcDev);

  if (EFI_ERROR (Status)) {
    MotorOff (FdcDev);
    FdcDev->ControllerState->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Set Media Parameter
  //
  FdcDev->BlkIo.Media->RemovableMedia = TRUE;
  FdcDev->BlkIo.Media->MediaPresent   = TRUE;
  FdcDev->BlkIo.Media->MediaId = 0;

  //
  // Check Media
  //
  Status = DisketChanged (FdcDev);

  if (Status == EFI_NO_MEDIA) {
    FdcDev->BlkIo.Media->MediaPresent = FALSE;
  } else if ((Status != EFI_MEDIA_CHANGED) &&
             (Status != EFI_SUCCESS)) {
    MotorOff (FdcDev);
    return Status;
  }

  //
  // Check Disk Write Protected
  //
  Status = SenseDrvStatus (FdcDev, 0);

  if (Status == EFI_WRITE_PROTECTED) {
    FdcDev->BlkIo.Media->ReadOnly = TRUE;
  } else if (Status == EFI_SUCCESS) {
    FdcDev->BlkIo.Media->ReadOnly = FALSE;
  } else {
    return EFI_DEVICE_ERROR;
  }

  MotorOff (FdcDev);

  //
  // Set Media Default Type
  //
  FdcDev->BlkIo.Media->BlockSize  = DISK_1440K_BYTEPERSECTOR;
  FdcDev->BlkIo.Media->LastBlock  = DISK_1440K_EOT * 2 * (DISK_1440K_MAXTRACKNUM + 1) - 1;

  return EFI_SUCCESS;
}

/**
  Reset the Floppy Logic Drive.
  
  @param  FdcDev FDC_BLK_IO_DEV * : A pointer to the FDC_BLK_IO_DEV
  
  @retval EFI_SUCCESS:    The Floppy Logic Drive is reset
  @retval EFI_DEVICE_ERROR: The Floppy Logic Drive is not functioning correctly and
                      can not be reset

**/
EFI_STATUS
FddReset (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  UINT8 Data;
  UINT8 StatusRegister0;
  UINT8 PresentCylinderNumber;
  UINTN Index;

  //
  // Report reset progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_REMOVABLE_MEDIA | EFI_P_PC_RESET,
    FdcDev->DevicePath
    );

  //
  // Reset specified Floppy Logic Drive according to FdcDev -> Disk
  // Set Digital Output Register(DOR) to do reset work
  //   bit0 & bit1 of DOR : Drive Select
  //   bit2 : Reset bit
  //   bit3 : DMA and Int bit
  // Reset : a "0" written to bit2 resets the FDC, this reset will remain
  //         active until
  //         a "1" is written to this bit.
  // Reset step 1:
  //         use bit0 & bit1 to  select the logic drive
  //         write "0" to bit2
  //
  Data = 0x0;
  Data = (UINT8) (Data | (SELECT_DRV & FdcDev->Disk));
  FdcWritePort (FdcDev, FDC_REGISTER_DOR, Data);

  //
  // wait some time,at least 120us
  //
  MicroSecondDelay (500);

  //
  // Reset step 2:
  //   write "1" to bit2
  //   write "1" to bit3 : enable DMA
  //
  Data |= 0x0C;
  FdcWritePort (FdcDev, FDC_REGISTER_DOR, Data);

  //
  // Experience value
  //
  MicroSecondDelay (2000);

  //
  // wait specified floppy logic drive is not busy
  //
  if (EFI_ERROR (FddWaitForBSYClear (FdcDev, 1))) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the Transfer Data Rate
  //
  FdcWritePort (FdcDev, FDC_REGISTER_CCR, 0x0);

  //
  // Experience value
  //
  MicroSecondDelay (100);

  //
  // Issue Sense interrupt command for each drive (total 4 drives)
  //
  for (Index = 0; Index < 4; Index++) {
    if (EFI_ERROR (SenseIntStatus (FdcDev, &StatusRegister0, &PresentCylinderNumber))) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // issue Specify command
  //
  if (EFI_ERROR (Specify (FdcDev))) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Turn the floppy disk drive's motor on.
  The drive's motor must be on before any command can be executed.
  
  @param[in] FdcDev  A pointer to the FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS            The drive's motor was turned on successfully
  @retval  EFI_DEVICE_ERROR       The drive is busy, so can not turn motor on
**/
EFI_STATUS
MotorOn (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;
  UINT8       DorData;

  //
  // Control of the floppy drive motors is a big pain. If motor is off, you have
  // to turn it on first. But you can not leave the motor on all the time, since
  // that would wear out the disk. On the other hand, if you turn the motor off
  // after each operation, the system performance will be awful. The compromise
  // used in this driver is to leave the motor on for 2 seconds after
  // each operation. If a new operation is started in that interval(2s),
  // the motor need not be turned on again. If no new operation is started,
  // a timer goes off and the motor is turned off
  //
  //
  // Cancel the timer
  //
  Status = gBS->SetTimer (FdcDev->Event, TimerCancel, 0);
  ASSERT_EFI_ERROR (Status);

  //
  // Get the motor status
  //
  DorData = FdcReadPort (FdcDev, FDC_REGISTER_DOR);

  if (((FdcDev->Disk == FdcDisk0) && ((DorData & 0x10) == 0x10)) ||
      ((FdcDev->Disk == FdcDisk1) && ((DorData & 0x21) == 0x21))
      ) {
    return EFI_SUCCESS;
  }
  //
  // The drive's motor is off, so need turn it on
  // first look at command and drive are busy or not
  //
  if (EFI_ERROR (FddWaitForBSYClear (FdcDev, 1))) {
    return EFI_DEVICE_ERROR;
  }
  //
  // for drive A: 1CH, drive B: 2DH
  //
  DorData = 0x0C;
  DorData = (UINT8) (DorData | (SELECT_DRV & FdcDev->Disk));
  if (FdcDev->Disk == FdcDisk0) {
    //
    // drive A
    //
    DorData |= DRVA_MOTOR_ON;
  } else {
    //
    // drive B
    //
    DorData |= DRVB_MOTOR_ON;
  }

  FdcWritePort (FdcDev, FDC_REGISTER_DOR, DorData);

  //
  // Experience value
  //
  MicroSecondDelay (4000);

  return EFI_SUCCESS;
}

/**
  Set a Timer and when Timer goes off, turn the motor off.
  
  @param[in] FdcDev  A pointer to the FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS            Set the Timer successfully
  @retval  EFI_INVALID_PARAMETER  Fail to Set the timer
**/
EFI_STATUS
MotorOff (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  //
  // Set the timer : 2s
  //
  return gBS->SetTimer (FdcDev->Event, TimerRelative, 20000000);
}

/**
  Detect whether the disk in the drive is changed or not.
  
  @param[in] FdcDev  A pointer to FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS        No disk media change
  @retval  EFI_DEVICE_ERROR   Fail to do the recalibrate or seek operation
  @retval  EFI_NO_MEDIA       No disk in the drive
  @retval  EFI_MEDIA_CHANGED  There is a new disk in the drive
**/
EFI_STATUS
DisketChanged (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Check change line
  //
  Data = FdcReadPort (FdcDev, FDC_REGISTER_DIR);

  //
  // Io delay
  //
  MicroSecondDelay (50);

  if ((Data & DIR_DCL) == 0x80) {
    //
    // disk change line is active
    //
    if (FdcDev->PresentCylinderNumber != 0) {
      Status = Recalibrate (FdcDev);
    } else {
      Status = Seek (FdcDev, 0x30);
    }

    if (EFI_ERROR (Status)) {
      FdcDev->ControllerState->NeedRecalibrate = TRUE;
      return EFI_DEVICE_ERROR;
      //
      // Fail to do the seek or recalibrate operation
      //
    }

    Data = FdcReadPort (FdcDev, FDC_REGISTER_DIR);

    //
    // Io delay
    //
    MicroSecondDelay (50);

    if ((Data & DIR_DCL) == 0x80) {
      return EFI_NO_MEDIA;
    }

    return EFI_MEDIA_CHANGED;
  }

  return EFI_SUCCESS;
}

/**
  Do the Specify command, this command sets DMA operation
  and the initial values for each of the three internal
  times: HUT, SRT and HLT.
  
  @param[in] FdcDev  Pointer to instance of FDC_BLK_IO_DEV
  
  @retval EFI_SUCCESS       Execute the Specify command successfully
  @retval EFI_DEVICE_ERROR  Fail to execute the command
**/
EFI_STATUS
Specify (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  FDD_SPECIFY_CMD Command;
  UINTN           Index;
  UINT8           *CommandPointer;

  ZeroMem (&Command, sizeof (FDD_SPECIFY_CMD));
  Command.CommandCode = SPECIFY_CMD;
  //
  // set SRT, HUT
  //
  Command.SrtHut = 0xdf;
  //
  // 0xdf;
  //
  // set HLT and DMA
  //
  Command.HltNd   = 0x02;

  CommandPointer  = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDD_SPECIFY_CMD); Index++) {
    if (EFI_ERROR (DataOutByte (FdcDev, CommandPointer++))) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Set the head of floppy drive to track 0.
 
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  @retval EFI_SUCCESS:    Execute the Recalibrate operation successfully
  @retval EFI_DEVICE_ERROR: Fail to execute the Recalibrate operation

**/
EFI_STATUS
Recalibrate (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  FDD_COMMAND_PACKET2 Command;
  UINTN               Index;
  UINT8               StatusRegister0;
  UINT8               PresentCylinderNumber;
  UINT8               *CommandPointer;
  UINT8               Count;

  Count = 2;

  while (Count > 0) {
    ZeroMem (&Command, sizeof (FDD_COMMAND_PACKET2));
    Command.CommandCode = RECALIBRATE_CMD;
    //
    // drive select
    //
    if (FdcDev->Disk == FdcDisk0) {
      Command.DiskHeadSel = 0;
      //
      // 0
      //
    } else {
      Command.DiskHeadSel = 1;
      //
      // 1
      //
    }

    CommandPointer = (UINT8 *) (&Command);
    for (Index = 0; Index < sizeof (FDD_COMMAND_PACKET2); Index++) {
      if (EFI_ERROR (DataOutByte (FdcDev, CommandPointer++))) {
        return EFI_DEVICE_ERROR;
      }
    }
    //
    // Experience value
    //
    MicroSecondDelay (250000);
    //
    // need modify according to 1.44M or 2.88M
    //
    if (EFI_ERROR (SenseIntStatus (FdcDev, &StatusRegister0, &PresentCylinderNumber))) {
      return EFI_DEVICE_ERROR;
    }

    if ((StatusRegister0 & 0xf0) == 0x20 && PresentCylinderNumber == 0) {
      FdcDev->PresentCylinderNumber             = 0;
      FdcDev->ControllerState->NeedRecalibrate  = FALSE;
      return EFI_SUCCESS;
    } else {
      Count--;
      if (Count == 0) {
        return EFI_DEVICE_ERROR;
      }
    }
  }
  //
  // end while
  //
  return EFI_SUCCESS;
}

/**
  Set the head of floppy drive to the new cylinder.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  @param  Lba EFI_LBA     : The logic block address want to seek
  
  @retval  EFI_SUCCESS:    Execute the Seek operation successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the Seek operation

**/
EFI_STATUS
Seek (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN EFI_LBA         Lba
  )
{
  FDD_SEEK_CMD  Command;
  UINT8         EndOfTrack;
  UINT8         Head;
  UINT8         Cylinder;
  UINT8         StatusRegister0;
  UINT8         *CommandPointer;
  UINT8         PresentCylinderNumber;
  UINTN         Index;
  UINT8         DelayTime;

  if (FdcDev->ControllerState->NeedRecalibrate) {
    if (EFI_ERROR (Recalibrate (FdcDev))) {
      FdcDev->ControllerState->NeedRecalibrate = TRUE;
      return EFI_DEVICE_ERROR;
    }
  }

  EndOfTrack = DISK_1440K_EOT;
  //
  // Calculate cylinder based on Lba and EOT
  //
  Cylinder = (UINT8) ((UINTN) Lba / EndOfTrack / 2);

  //
  // if the destination cylinder is the present cylinder, unnecessary to do the
  // seek operation
  //
  if (FdcDev->PresentCylinderNumber == Cylinder) {
    return EFI_SUCCESS;
  }
  //
  // Calculate the head : 0 or 1
  //
  Head = (UINT8) ((UINTN) Lba / EndOfTrack % 2);

  ZeroMem (&Command, sizeof (FDD_SEEK_CMD));
  Command.CommandCode = SEEK_CMD;
  if (FdcDev->Disk == FdcDisk0) {
    Command.DiskHeadSel = 0;
    //
    // 0
    //
  } else {
    Command.DiskHeadSel = 1;
    //
    // 1
    //
  }

  Command.DiskHeadSel = (UINT8) (Command.DiskHeadSel | (Head << 2));
  Command.NewCylinder = Cylinder;

  CommandPointer      = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDD_SEEK_CMD); Index++) {
    if (EFI_ERROR (DataOutByte (FdcDev, CommandPointer++))) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // Io delay
  //
  MicroSecondDelay (100);

  //
  // Calculate waiting time
  //
  if (FdcDev->PresentCylinderNumber > Cylinder) {
    DelayTime = (UINT8) (FdcDev->PresentCylinderNumber - Cylinder);
  } else {
    DelayTime = (UINT8) (Cylinder - FdcDev->PresentCylinderNumber);
  }

  MicroSecondDelay ((DelayTime + 1) * 4000);

  if (EFI_ERROR (SenseIntStatus (FdcDev, &StatusRegister0, &PresentCylinderNumber))) {
    return EFI_DEVICE_ERROR;
  }

  if ((StatusRegister0 & 0xf0) == 0x20) {
    FdcDev->PresentCylinderNumber = Command.NewCylinder;
    return EFI_SUCCESS;
  } else {
    FdcDev->ControllerState->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
}

/**
  Do the Sense Interrupt Status command, this command
  resets the interrupt signal.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  @param  StatusRegister0 UINT8 *: Be used to save Status Register 0 read from FDC
  @param  PresentCylinderNumber  UINT8 *: Be used to save present cylinder number
                                    read from FDC
  
  @retval  EFI_SUCCESS:    Execute the Sense Interrupt Status command successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the command

**/
EFI_STATUS
SenseIntStatus (
  IN     FDC_BLK_IO_DEV  *FdcDev,
  IN OUT UINT8           *StatusRegister0,
  IN OUT UINT8           *PresentCylinderNumber
  )
{
  UINT8 Command;

  Command = SENSE_INT_STATUS_CMD;
  if (EFI_ERROR (DataOutByte (FdcDev, &Command))) {
    return EFI_DEVICE_ERROR;
  }

  if (EFI_ERROR (DataInByte (FdcDev, StatusRegister0))) {
    return EFI_DEVICE_ERROR;
  }

  if (EFI_ERROR (DataInByte (FdcDev, PresentCylinderNumber))) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Do the Sense Drive Status command.
  
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  @param  Lba EFI_LBA     : Logic block address
  
  @retval  EFI_SUCCESS:    Execute the Sense Drive Status command successfully
  @retval  EFI_DEVICE_ERROR: Fail to execute the command
  @retval  EFI_WRITE_PROTECTED:The disk is write protected

**/
EFI_STATUS
SenseDrvStatus (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN EFI_LBA         Lba
  )
{
  FDD_COMMAND_PACKET2 Command;
  UINT8               Head;
  UINT8               EndOfTrack;
  UINTN               Index;
  UINT8               StatusRegister3;
  UINT8               *CommandPointer;

  //
  // Sense Drive Status command obtains drive status information,
  // it has not execution phase and goes directly to the result phase from the
  // command phase, Status Register 3 contains the drive status information
  //
  ZeroMem (&Command, sizeof (FDD_COMMAND_PACKET2));
  Command.CommandCode = SENSE_DRV_STATUS_CMD;

  if (FdcDev->Disk == FdcDisk0) {
    Command.DiskHeadSel = 0;
  } else {
    Command.DiskHeadSel = 1;
  }

  EndOfTrack  = DISK_1440K_EOT;
  Head        = (UINT8) ((UINTN) Lba / EndOfTrack % 2);
  Command.DiskHeadSel = (UINT8) (Command.DiskHeadSel | (Head << 2));

  CommandPointer = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDD_COMMAND_PACKET2); Index++) {
    if (EFI_ERROR (DataOutByte (FdcDev, CommandPointer++))) {
      return EFI_DEVICE_ERROR;
    }
  }

  if (EFI_ERROR (DataInByte (FdcDev, &StatusRegister3))) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Io delay
  //
  MicroSecondDelay (50);

  //
  // Check Status Register 3 to get drive status information
  //
  return CheckStatus3 (StatusRegister3);
}

/**
  Update the disk media properties and if necessary reinstall Block I/O interface.
 
  @param  FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  
  @retval  EFI_SUCCESS:    Do the operation successfully
  @retval  EFI_DEVICE_ERROR: Fail to the operation

**/
EFI_STATUS
DetectMedia (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Reset;
  BOOLEAN     ReadOnlyLastTime;
  BOOLEAN     MediaPresentLastTime;

  Reset                = FALSE;
  ReadOnlyLastTime     = FdcDev->BlkIo.Media->ReadOnly;
  MediaPresentLastTime = FdcDev->BlkIo.Media->MediaPresent;

  //
  // Check disk change
  //
  Status = DisketChanged (FdcDev);

  if (Status == EFI_MEDIA_CHANGED) {
    FdcDev->BlkIo.Media->MediaId++;
    FdcDev->BlkIo.Media->MediaPresent = TRUE;
    Reset = TRUE;
  } else if (Status == EFI_NO_MEDIA) {
    FdcDev->BlkIo.Media->MediaPresent = FALSE;
  } else if (Status != EFI_SUCCESS) {
    MotorOff (FdcDev);
    return Status;
    //
    // EFI_DEVICE_ERROR
    //
  }

  if (FdcDev->BlkIo.Media->MediaPresent) {
    //
    // Check disk write protected
    //
    Status = SenseDrvStatus (FdcDev, 0);
    if (Status == EFI_WRITE_PROTECTED) {
      FdcDev->BlkIo.Media->ReadOnly = TRUE;
    } else {
      FdcDev->BlkIo.Media->ReadOnly = FALSE;
    }
  }

  if (FdcDev->BlkIo.Media->MediaPresent && (ReadOnlyLastTime != FdcDev->BlkIo.Media->ReadOnly)) {
    Reset = TRUE;
  }

  if (MediaPresentLastTime != FdcDev->BlkIo.Media->MediaPresent) {
    Reset = TRUE;
  }

  if (Reset) {
    Status = gBS->ReinstallProtocolInterface (
                    FdcDev->Handle,
                    &gEfiBlockIoProtocolGuid,
                    &FdcDev->BlkIo,
                    &FdcDev->BlkIo
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Set the data rate and so on.
 
  @param  FdcDev  A pointer to FDC_BLK_IO_DEV

  @retval EFI_SUCCESS success to set the data rate
**/
EFI_STATUS
Setup (
  IN FDC_BLK_IO_DEV  *FdcDev
  )
{
  EFI_STATUS  Status;

  //
  // Set data rate 500kbs
  //
  FdcWritePort (FdcDev, FDC_REGISTER_CCR, 0x0);

  //
  // Io delay
  //
  MicroSecondDelay (50);

  Status = Specify (FdcDev);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Read or Write a number of blocks in the same cylinder.
 
  @param  FdcDev      A pointer to FDC_BLK_IO_DEV
  @param  HostAddress device address 
  @param  Lba         The starting logic block address to read from on the device
  @param  NumberOfBlocks The number of block wanted to be read or write
  @param  Read        Operation type: read or write
  
  @retval EFI_SUCCESS Success operate

**/
EFI_STATUS
ReadWriteDataSector (
  IN  FDC_BLK_IO_DEV  *FdcDev,
  IN  VOID            *HostAddress,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks,
  IN  BOOLEAN         Read
  )
{
  EFI_STATUS                                    Status;
  FDD_COMMAND_PACKET1                           Command;
  FDD_RESULT_PACKET                             Result;
  UINTN                                         Index;
  UINTN                                         Times;
  UINT8                                         *CommandPointer;

  EFI_PHYSICAL_ADDRESS                          DeviceAddress;
  EFI_ISA_IO_PROTOCOL                           *IsaIo;
  UINTN                                         NumberofBytes;
  VOID                                          *Mapping;
  EFI_ISA_IO_PROTOCOL_OPERATION                 Operation;
  EFI_STATUS                                    Status1;
  UINT8                                         Channel;
  EFI_ISA_ACPI_RESOURCE                         *ResourceItem;
  UINT32                                        Attribute;

  Status = Seek (FdcDev, Lba);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Map Dma
  //
  IsaIo         = FdcDev->IsaIo;
  NumberofBytes = NumberOfBlocks * 512;
  if (Read == READ) {
    Operation = EfiIsaIoOperationSlaveWrite;
  } else {
    Operation = EfiIsaIoOperationSlaveRead;
  }

  ResourceItem  = IsaIo->ResourceList->ResourceItem;
  Index         = 0;
  while (ResourceItem[Index].Type != EfiIsaAcpiResourceEndOfList) {
    if (ResourceItem[Index].Type == EfiIsaAcpiResourceDma) {
      break;
    }

    Index++;
  }

  if (ResourceItem[Index].Type == EfiIsaAcpiResourceEndOfList) {
    return EFI_DEVICE_ERROR;
  }

  Channel   = (UINT8) IsaIo->ResourceList->ResourceItem[Index].StartRange;
  Attribute = IsaIo->ResourceList->ResourceItem[Index].Attribute;

  Status1 = IsaIo->Map (
                    IsaIo,
                    Operation,
                    Channel,
                    Attribute,
                    HostAddress,
                    &NumberofBytes,
                    &DeviceAddress,
                    &Mapping
                    );
  if (EFI_ERROR (Status1)) {
    return Status1;
  }

  //
  // Allocate Read or Write command packet
  //
  ZeroMem (&Command, sizeof (FDD_COMMAND_PACKET1));
  if (Read == READ) {
    Command.CommandCode = READ_DATA_CMD | CMD_MT | CMD_MFM | CMD_SK;
  } else {
    Command.CommandCode = WRITE_DATA_CMD | CMD_MT | CMD_MFM;
  }

  FillPara (FdcDev, Lba, &Command);

  //
  // Write command bytes to FDC
  //
  CommandPointer = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDD_COMMAND_PACKET1); Index++) {
    if (EFI_ERROR (DataOutByte (FdcDev, CommandPointer++))) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // wait for some time
  //
  Times = (STALL_1_SECOND / 50) + 1;
  do {
    if ((FdcReadPort (FdcDev, FDC_REGISTER_MSR) & 0xc0) == 0xc0) {
      break;
    }

    MicroSecondDelay (50);
    Times = Times - 1;
  } while (Times > 0);

  if (Times == 0) {
    return EFI_TIMEOUT;
  }
  //
  // Read result bytes from FDC
  //
  CommandPointer = (UINT8 *) (&Result);
  for (Index = 0; Index < sizeof (FDD_RESULT_PACKET); Index++) {
    if (EFI_ERROR (DataInByte (FdcDev, CommandPointer++))) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // Flush before Unmap
  //
  if (Read == READ) {
    Status1 = IsaIo->Flush (IsaIo);
    if (EFI_ERROR (Status1)) {
      return Status1;
    }
  }
  //
  // Unmap Dma
  //
  Status1 = IsaIo->Unmap (IsaIo, Mapping);
  if (EFI_ERROR (Status1)) {
    return Status1;
  }

  return CheckResult (&Result, FdcDev);
}

/**
  Fill in FDD command's parameter.
  
  @param FdcDev   Pointer to instance of FDC_BLK_IO_DEV
  @param Lba      The starting logic block address to read from on the device
  @param Command  FDD command

**/
VOID
FillPara (
  IN  FDC_BLK_IO_DEV       *FdcDev,
  IN  EFI_LBA              Lba,
  IN  FDD_COMMAND_PACKET1  *Command
  )
{
  UINT8 EndOfTrack;

  //
  // Get EndOfTrack from the Para table
  //
  EndOfTrack = DISK_1440K_EOT;

  //
  // Fill the command parameter
  //
  if (FdcDev->Disk == FdcDisk0) {
    Command->DiskHeadSel = 0;
  } else {
    Command->DiskHeadSel = 1;
  }

  Command->Cylinder = (UINT8) ((UINTN) Lba / EndOfTrack / 2);
  Command->Head     = (UINT8) ((UINTN) Lba / EndOfTrack % 2);
  Command->Sector   = (UINT8) ((UINT8) ((UINTN) Lba % EndOfTrack) + 1);
  Command->DiskHeadSel = (UINT8) (Command->DiskHeadSel | (Command->Head << 2));
  Command->Number     = DISK_1440K_NUMBER;
  Command->EndOfTrack = DISK_1440K_EOT;
  Command->GapLength  = DISK_1440K_GPL;
  Command->DataLength = DISK_1440K_DTL;
}

/**
  Read result byte from Data Register of FDC.
  
  @param FdcDev   Pointer to instance of FDC_BLK_IO_DEV
  @param Pointer  Buffer to store the byte read from FDC
  
  @retval EFI_SUCCESS       Read result byte from FDC successfully
  @retval EFI_DEVICE_ERROR  The FDC is not ready to be read

**/
EFI_STATUS
DataInByte (
  IN  FDC_BLK_IO_DEV  *FdcDev,
  OUT UINT8           *Pointer
  )
{
  UINT8 Data;

  //
  // wait for 1ms and detect the FDC is ready to be read
  //
  if (EFI_ERROR (FddDRQReady (FdcDev, DATA_IN, 1))) {
    return EFI_DEVICE_ERROR;
    //
    // is not ready
    //
  }

  Data = FdcReadPort (FdcDev, FDC_REGISTER_DTR);

  //
  // Io delay
  //
  MicroSecondDelay (50);

  *Pointer = Data;
  return EFI_SUCCESS;
}

/**
  Write command byte to Data Register of FDC.
  
  @param FdcDev  Pointer to instance of FDC_BLK_IO_DEV
  @param Pointer Be used to save command byte written to FDC
  
  @retval  EFI_SUCCESS:    Write command byte to FDC successfully
  @retval  EFI_DEVICE_ERROR: The FDC is not ready to be written

**/
EFI_STATUS
DataOutByte (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT8           *Pointer
  )
{
  UINT8 Data;

  //
  // wait for 1ms and detect the FDC is ready to be written
  //
  if (EFI_ERROR (FddDRQReady (FdcDev, DATA_OUT, 1))) {
    //
    // Not ready
    //
    return EFI_DEVICE_ERROR;
  }

  Data = *Pointer;

  FdcWritePort (FdcDev, FDC_REGISTER_DTR, Data);

  //
  // Io delay
  //
  MicroSecondDelay (50);

  return EFI_SUCCESS;
}

/**
  Detect the specified floppy logic drive is busy or not within a period of time.
  
  @param FdcDev           Indicate it is drive A or drive B
  @param Timeout          The time period for waiting
  
  @retval EFI_SUCCESS:  The drive and command are not busy
  @retval EFI_TIMEOUT:  The drive or command is still busy after a period time that
                        set by Timeout

**/
EFI_STATUS
FddWaitForBSYClear (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINTN           Timeout
  )
{
  UINTN Delay;
  UINT8 StatusRegister;
  UINT8 Mask;

  //
  // How to determine drive and command are busy or not: by the bits of
  // Main Status Register
  // bit0: Drive 0 busy (drive A)
  // bit1: Drive 1 busy (drive B)
  // bit4: Command busy
  //
  //
  // set mask: for drive A set bit0 & bit4; for drive B set bit1 & bit4
  //
  Mask  = (UINT8) ((FdcDev->Disk == FdcDisk0 ? MSR_DAB : MSR_DBB) | MSR_CB);

  Delay = ((Timeout * STALL_1_MSECOND) / 50) + 1;
  do {
    StatusRegister = FdcReadPort (FdcDev, FDC_REGISTER_MSR);
    if ((StatusRegister & Mask) == 0x00) {
      break;
      //
      // not busy
      //
    }

    MicroSecondDelay (50);
    Delay = Delay - 1;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Determine whether FDC is ready to write or read.
  
  @param  FdcDev Pointer to instance of FDC_BLK_IO_DEV
  @param  Dio BOOLEAN:      Indicate the FDC is waiting to write or read
  @param  Timeout           The time period for waiting
  
  @retval EFI_SUCCESS:  FDC is ready to write or read
  @retval EFI_NOT_READY:  FDC is not ready within the specified time period

**/
EFI_STATUS
FddDRQReady (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN BOOLEAN         Dio,
  IN UINTN           Timeout
  )
{
  UINTN Delay;
  UINT8 StatusRegister;
  UINT8 DataInOut;

  //
  // Before writing to FDC or reading from FDC, the Host must examine
  // the bit7(RQM) and bit6(DIO) of the Main Status Register.
  // That is to say:
  //  command bytes can not be written to Data Register
  //  unless RQM is 1 and DIO is 0
  //  result bytes can not be read from Data Register
  //  unless RQM is 1 and DIO is 1
  //
  DataInOut = (UINT8) (Dio << 6);
  //
  // in order to compare bit6
  //
  Delay = ((Timeout * STALL_1_MSECOND) / 50) + 1;
  do {
    StatusRegister = FdcReadPort (FdcDev, FDC_REGISTER_MSR);
    if ((StatusRegister & MSR_RQM) == MSR_RQM && (StatusRegister & MSR_DIO) == DataInOut) {
      break;
      //
      // FDC is ready
      //
    }

    MicroSecondDelay (50);
    //
    // Stall for 50 us
    //
    Delay = Delay - 1;
  } while (Delay > 0);

  if (Delay == 0) {
    return EFI_NOT_READY;
    //
    // FDC is not ready within the specified time period
    //
  }

  return EFI_SUCCESS;
}

/**
  Set FDC control structure's attribute according to result. 

  @param Result  Point to result structure
  @param FdcDev  FDC control structure

  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_DEVICE_ERROR - GC_TODO: Add description for return value
  @retval EFI_SUCCESS - GC_TODO: Add description for return value

**/
EFI_STATUS
CheckResult (
  IN     FDD_RESULT_PACKET  *Result,
  IN OUT FDC_BLK_IO_DEV     *FdcDev
  )
{
  //
  // Check Status Register0
  //
  if ((Result->Status0 & STS0_IC) != IC_NT) {
    if ((Result->Status0 & STS0_SE) == 0x20) {
      //
      // seek error
      //
      FdcDev->ControllerState->NeedRecalibrate = TRUE;
    }

    FdcDev->ControllerState->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register1
  //
  if ((Result->Status1 & (STS1_EN | STS1_DE | STS1_OR | STS1_ND | STS1_NW | STS1_MA)) != 0) {
    FdcDev->ControllerState->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register2
  //
  if ((Result->Status2 & (STS2_CM | STS2_DD | STS2_WC | STS2_BC | STS2_MD)) != 0) {
    FdcDev->ControllerState->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Check the drive status information.
  
  @param StatusRegister3  the value of Status Register 3
  
  @retval EFI_SUCCESS           The disk is not write protected
  @retval EFI_WRITE_PROTECTED:  The disk is write protected

**/
EFI_STATUS
CheckStatus3 (
  IN UINT8 StatusRegister3
  )
{
  if ((StatusRegister3 & STS3_WP) != 0) {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_SUCCESS;
}

/**
  Calculate the number of block in the same cylinder according to LBA.
  
  @param FdcDev FDC_BLK_IO_DEV *: A pointer to FDC_BLK_IO_DEV
  @param LBA EFI_LBA:      The starting logic block address
  @param NumberOfBlocks UINTN: The number of blocks
  
  @return The number of blocks in the same cylinder which the starting
        logic block address is LBA

**/
UINTN
GetTransferBlockCount (
  IN  FDC_BLK_IO_DEV  *FdcDev,
  IN  EFI_LBA         LBA,
  IN  UINTN           NumberOfBlocks
  )
{
  UINT8 EndOfTrack;
  UINT8 Head;
  UINT8 SectorsInTrack;

  //
  // Calculate the number of block in the same cylinder
  //
  EndOfTrack      = DISK_1440K_EOT;
  Head            = (UINT8) ((UINTN) LBA / EndOfTrack % 2);

  SectorsInTrack  = (UINT8) (EndOfTrack * (2 - Head) - (UINT8) ((UINTN) LBA % EndOfTrack));
  if (SectorsInTrack < NumberOfBlocks) {
    return SectorsInTrack;
  } else {
    return NumberOfBlocks;
  }
}

/**
  When the Timer(2s) off, turn the drive's motor off.
  
  @param Event EFI_EVENT: Event(the timer) whose notification function is being
                     invoked
  @param Context VOID *:  Pointer to the notification function's context

**/
VOID
EFIAPI
FddTimerProc (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  FDC_BLK_IO_DEV  *FdcDev;
  UINT8           Data;

  FdcDev = (FDC_BLK_IO_DEV *) Context;

  //
  // Get the motor status
  //
  Data = FdcReadPort (FdcDev, FDC_REGISTER_DOR);

  if (((FdcDev->Disk == FdcDisk0) && ((Data & 0x10) != 0x10)) ||
      ((FdcDev->Disk == FdcDisk1) && ((Data & 0x21) != 0x21))
      ) {
    return ;
  }
  //
  // the motor is on, so need motor off
  //
  Data = 0x0C;
  Data = (UINT8) (Data | (SELECT_DRV & FdcDev->Disk));
  FdcWritePort (FdcDev, FDC_REGISTER_DOR, Data);
  MicroSecondDelay (500);
}

/**
  Read an I/O port of FDC.
 
  @param[in] FdcDev  A pointer to FDC_BLK_IO_DEV.
  @param[in] Offset  The address offset of the I/O port.

  @retval  8-bit data read from the I/O port.
**/
UINT8
FdcReadPort (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT32          Offset
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  Status = FdcDev->IsaIo->Io.Read (
                            FdcDev->IsaIo,
                            EfiIsaIoWidthUint8,
                            FdcDev->BaseAddress + Offset,
                            1,
                            &Data
                            );
  ASSERT_EFI_ERROR (Status);

  return Data;
}

/**
  Write an I/O port of FDC.
 
  @param[in] FdcDev  A pointer to FDC_BLK_IO_DEV
  @param[in] Offset  The address offset of the I/O port
  @param[in] Data    8-bit Value written to the I/O port
**/
VOID
FdcWritePort (
  IN FDC_BLK_IO_DEV  *FdcDev,
  IN UINT32          Offset,
  IN UINT8           Data
  )
{
  EFI_STATUS  Status;

  Status = FdcDev->IsaIo->Io.Write (
                            FdcDev->IsaIo,
                            EfiIsaIoWidthUint8,
                            FdcDev->BaseAddress + Offset,
                            1,
                            &Data
                            );
  ASSERT_EFI_ERROR (Status);
}

