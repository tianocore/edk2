/** @file
Floppy Peim to support Recovery function from Floppy device.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "FloppyPeim.h"


PEI_DMA_TABLE      mRegisterTable[] = {
  //
  // DMA2: Clear Byte Ptr, Enable
  //
  {
    R_8237_DMA_CBPR_CH4_7,
    0
  },
  {
    R_8237_DMA_COMMAND_CH4_7,
    0
  },
  //
  // DMA1: Clear Byte Ptr, Enable
  //
  {
    R_8237_DMA_CBPR_CH0_3,
    0
  },
  {
    R_8237_DMA_COMMAND_CH0_3,
    0
  },
  //
  // Configure Channel 4 for Cascade Mode
  // Clear DMA Request and enable DREQ
  //
  {
    R_8237_DMA_CHMODE_CH4_7,
    V_8237_DMA_CHMODE_CASCADE | 0
  },
  {
    R_8237_DMA_STA_CH4_7,
    0
  },
  {
    R_8237_DMA_WRSMSK_CH4_7,
    0
  },
  //
  // Configure DMA1 (Channels 0-3) for Single Mode
  // Clear DMA Request and enable DREQ
  //
  {
    R_8237_DMA_CHMODE_CH0_3,
    V_8237_DMA_CHMODE_SINGLE | 0
  },
  {
    R_8237_DMA_STA_CH0_3,
    0
  },
  {
    R_8237_DMA_WRSMSK_CH0_3,
    0
  },
  {
    R_8237_DMA_CHMODE_CH0_3,
    V_8237_DMA_CHMODE_SINGLE | 1
  },
  {
    R_8237_DMA_STA_CH0_3,
    1
  },
  {
    R_8237_DMA_WRSMSK_CH0_3,
    1
  },
  {
    R_8237_DMA_CHMODE_CH0_3,
    V_8237_DMA_CHMODE_SINGLE | 2
  },
  {
    R_8237_DMA_STA_CH0_3,
    2
  },
  {
    R_8237_DMA_WRSMSK_CH0_3,
    2
  },
  {
    R_8237_DMA_CHMODE_CH0_3,
    V_8237_DMA_CHMODE_SINGLE | 3
  },
  {
    R_8237_DMA_STA_CH0_3,
    3
  },
  {
    R_8237_DMA_WRSMSK_CH0_3,
    3
  },
  //
  // Configure DMA2 (Channels 5-7) for Single Mode
  // Clear DMA Request and enable DREQ
  //
  {
    R_8237_DMA_CHMODE_CH4_7,
    V_8237_DMA_CHMODE_SINGLE | 1
  },
  {
    R_8237_DMA_STA_CH4_7,
    1
  },
  {
    R_8237_DMA_WRSMSK_CH4_7,
    1
  },
  {
    R_8237_DMA_CHMODE_CH4_7,
    V_8237_DMA_CHMODE_SINGLE | 2
  },
  {
    R_8237_DMA_STA_CH4_7,
    2
  },
  {
    R_8237_DMA_WRSMSK_CH4_7,
    2
  },
  {
    R_8237_DMA_CHMODE_CH4_7,
    V_8237_DMA_CHMODE_SINGLE | 3
  },
  {
    R_8237_DMA_STA_CH4_7,
    3
  },
  {
    R_8237_DMA_WRSMSK_CH4_7,
    3
  }
};

//
// Table of diskette parameters of various diskette types 
//
DISKET_PARA_TABLE  DiskPara[9] = {
  {
    0x09,
    0x50,
    0xff,
    0x2,
    0x27,
    0x4,
    0x25,
    0x14,
    0x80
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x27,
    0x4,
    0x25,
    0x0f,
    0x40
  },
  {
    0x0f,
    0x54,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x09,
    0x50,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x12,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x09,
    0x2a,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x80
  },
  {
    0x12,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0x0
  },
  {
    0x24,
    0x1b,
    0xff,
    0x2,
    0x4f,
    0x4,
    0x25,
    0x0f,
    0xc0
  }
};

//
// Byte per sector corresponding to various device types.
//
UINTN    BytePerSector[6] = { 0, 256, 512, 1024, 2048, 4096 };

FDC_BLK_IO_DEV mBlockIoDevTemplate = {
  FDC_BLK_IO_DEV_SIGNATURE,
  {
    FdcGetNumberOfBlockDevices,
    FdcGetBlockDeviceMediaInfo,
    FdcReadBlocks,
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiVirtualBlockIoPpiGuid,
    NULL
  },
  0,
  {{0}}
};

/**
  Wait and check if bits for DIO and RQM of FDC Main Status Register
  indicates FDC is ready for read or write.

  Before writing to FDC or reading from FDC, the Host must examine
  the bit7(RQM) and bit6(DIO) of the Main Status Register.
  That is to say:
   Command bytes can not be written to Data Register unless RQM is 1 and DIO is 0.
   Result bytes can not be read from Data Register unless RQM is 1 and DIO is 1.

  @param  FdcBlkIoDev       Instance of FDC_BLK_IO_DEV.
  @param  DataIn            Indicates data input or output.
                            TRUE means input.
                            FALSE means output.
  @param  TimeoutInMseconds  Timeout value to wait.
  
  @retval EFI_SUCCESS       FDC is ready.
  @retval EFI_NOT_READY     FDC is not ready within the specified time period.

**/
EFI_STATUS
FdcDRQReady (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN BOOLEAN          DataIn,
  IN UINTN            TimeoutInMseconds
  )
{
  UINTN   Delay;
  UINT8   StatusRegister;
  UINT8   BitInOut;

  //
  // Check bit6 of Main Status Register.
  //
  BitInOut = 0;
  if (DataIn) {
    BitInOut = BIT6;
  }

  Delay = ((TimeoutInMseconds * STALL_1_MSECOND) / FDC_CHECK_INTERVAL) + 1;
  do {
    StatusRegister = IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_MSR));
    if ((StatusRegister & MSR_RQM) == MSR_RQM && (StatusRegister & MSR_DIO) == BitInOut) {
      //
      // FDC is ready
      //
      break;
    }

    MicroSecondDelay (FDC_SHORT_DELAY);
  } while (--Delay > 0);

  if (Delay == 0) {
    //
    // FDC is not ready within the specified time period
    //
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Read a byte from FDC data register.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Pointer          Pointer to buffer to hold data read from FDC.

  @retval EFI_SUCCESS      Byte successfully read.
  @retval EFI_DEVICE_ERROR FDC is not ready.

**/
EFI_STATUS
DataInByte (
  IN  FDC_BLK_IO_DEV   *FdcBlkIoDev,
  OUT UINT8            *Pointer
  )
{
  UINT8 Data;

  //
  // Wait for 1ms and detect the FDC is ready to be read
  //
  if (FdcDRQReady (FdcBlkIoDev, TRUE, 1) != EFI_SUCCESS) {
    //
    // FDC is not ready.
    //
    return EFI_DEVICE_ERROR;
  }

  Data = IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DTR));
  MicroSecondDelay (FDC_SHORT_DELAY);
  *Pointer = Data;

  return EFI_SUCCESS;
}

/**
  Write a byte to FDC data register.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Pointer          Pointer to data to write.

  @retval EFI_SUCCESS      Byte successfully written.
  @retval EFI_DEVICE_ERROR FDC is not ready.

**/
EFI_STATUS
DataOutByte (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            *Pointer
  )
{
  UINT8 Data;

  //
  // Wait for 1ms and detect the FDC is ready to be written
  //
  if (FdcDRQReady (FdcBlkIoDev, FALSE, 1) != EFI_SUCCESS) {
    //
    // FDC is not ready.
    //
    return EFI_DEVICE_ERROR;
  }

  Data = *Pointer;
  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DTR), Data);
  MicroSecondDelay (FDC_SHORT_DELAY);

  return EFI_SUCCESS;
}

/**
  Get Sts0 and Pcn status from FDC

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV
  @param  Sts0             Value of Sts0
  @param  Pcn              Value of Pcn

  @retval EFI_SUCCESS      Successfully retrieved status value of Sts0 and Pcn.
  @retval EFI_DEVICE_ERROR Fail to send SENSE_INT_STATUS_CMD.
  @retval EFI_DEVICE_ERROR Fail to read Sts0.
  @retval EFI_DEVICE_ERROR Fail to read Pcn.

**/
EFI_STATUS
SenseIntStatus (
  IN  FDC_BLK_IO_DEV   *FdcBlkIoDev,
  OUT UINT8            *Sts0,
  OUT UINT8            *Pcn
  )
{
  UINT8 Command;

  Command = SENSE_INT_STATUS_CMD;

  if (DataOutByte (FdcBlkIoDev, &Command) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (DataInByte (FdcBlkIoDev, Sts0) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (DataInByte (FdcBlkIoDev, Pcn) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Issue Specify command.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.

  @retval EFI_SUCCESS      Specify command successfully issued.
  @retval EFI_DEVICE_ERROR FDC device has errors.

**/
EFI_STATUS
Specify (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  )
{
  FDC_SPECIFY_CMD Command;
  UINTN           Index;
  UINT8           *Pointer;

  ZeroMem (&Command, sizeof (FDC_SPECIFY_CMD));
  Command.CommandCode = SPECIFY_CMD;
  //
  // set SRT, HUT
  //
  Command.SrtHut = 0xdf;
  //
  // 0xdf;
  // set HLT and DMA
  //
  Command.HltNd = 0x02;

  Pointer            = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDC_SPECIFY_CMD); Index++) {
    if (DataOutByte (FdcBlkIoDev, Pointer++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Wait until busy bit is cleared.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  DevPos           Position of FDC (Driver A or B)
  @param  TimeoutInMseconds Timeout value to wait.

  @retval EFI_SUCCESS      Busy bit has been cleared before timeout.
  @retval EFI_TIMEOUT      Time goes out before busy bit is cleared.

**/
EFI_STATUS
FdcWaitForBSYClear (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos,
  IN UINTN            TimeoutInMseconds
  )
{
  UINTN   Delay;
  UINT8   StatusRegister;
  UINT8   Mask;

  //
  // How to determine drive and command are busy or not: by the bits of Main Status Register
  // bit0: Drive 0 busy (drive A)
  // bit1: Drive 1 busy (drive B)
  // bit4: Command busy
  //
  // set mask: for drive A set bit0 & bit4; for drive B set bit1 & bit4
  //
  Mask  = (UINT8) ((DevPos == 0 ? MSR_DAB : MSR_DBB) | MSR_CB);

  Delay = ((TimeoutInMseconds * STALL_1_MSECOND) / FDC_CHECK_INTERVAL) + 1;

  do {
    StatusRegister = IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_MSR));

    if ((StatusRegister & Mask) == 0x00) {
      //
      // not busy
      //
      break;
    }

    MicroSecondDelay (FDC_SHORT_DELAY);
  } while (--Delay > 0);

  if (Delay == 0) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Reset FDC device.

  @param  FdcBlkIoDev  Instance of FDC_BLK_IO_DEV
  @param  DevPos       Index of FDC device.

  @retval EFI_SUCCESS      FDC device successfully reset.
  @retval EFI_DEVICE_ERROR Fail to reset FDC device.

**/
EFI_STATUS
FdcReset (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN UINT8            DevPos
  )
{
  UINT8 Data;
  UINT8 Sts0;
  UINT8 Pcn;
  UINTN Index;

  //
  // Reset specified Floppy Logic Drive according to Fdd -> Disk
  // Set Digital Output Register(DOR) to do reset work
  //    bit0 & bit1 of DOR : Drive Select
  //    bit2 : Reset bit
  //    bit3 : DMA and Int bit
  // Reset : A "0" written to bit2 resets the FDC, this reset will remain active until
  //       a "1" is written to this bit.
  // Reset step 1:
  //    use bit0 & bit1 to  select the logic drive
  //    write "0" to bit2
  //
  Data = 0x0;
  Data = (UINT8) (Data | (SELECT_DRV & DevPos));
  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DOR), Data);

  //
  // Wait some time, at least 120us.
  //
  MicroSecondDelay (FDC_RESET_DELAY);
  //
  // Reset step 2:
  //    write "1" to bit2
  //    write "1" to bit3 : enable DMA
  //
  Data |= 0x0C;
  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DOR), Data);

  MicroSecondDelay (FDC_RESET_DELAY);

  //
  // Wait until specified floppy logic drive is not busy
  //
  if (FdcWaitForBSYClear (FdcBlkIoDev, DevPos, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the Transfer Data Rate
  //
  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_CCR), 0x0);

  MicroSecondDelay (FDC_MEDIUM_DELAY);

  //
  // Issue Sense interrupt command for each drive (totally 4 drives)
  //
  for (Index = 0; Index < 4; Index++) {
    if (SenseIntStatus (FdcBlkIoDev, &Sts0, &Pcn) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }
  //
  // Issue Specify command
  //
  if (Specify (FdcBlkIoDev) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Turn on the motor of floppy drive.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Info             Information of floppy device.

  @retval EFI_SUCCESS      Motor is successfully turned on.
  @retval EFI_SUCCESS      Motor is already on.
  @retval EFI_DEVICE_ERROR Busy bit of FDC cannot be cleared.

**/
EFI_STATUS
MotorOn (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
{
  UINT8 Data;
  UINT8 DevPos;

  //
  // Control of the floppy drive motors is a big pain. If motor is off, you have to turn it
  // on first. But you can not leave the motor on all the time, since that would wear out the
  // disk. On the other hand, if you turn the motor off after each operation, the system performance
  // will be awful. The compromise used in this driver is to leave the motor on for 2 seconds after
  // each operation. If a new operation is started in that interval(2s), the motor need not be
  // turned on again. If no new operation is started, a timer goes off and the motor is turned off.
  //
  DevPos = Info->DevPos;

  //
  // If the Motor is already on, just return EFI_SUCCESS.
  //
  if (Info->MotorOn) {
    return EFI_SUCCESS;
  }
  //
  // The drive's motor is off, so need turn it on.
  // First check if command and drive are busy or not.
  //
  if (FdcWaitForBSYClear (FdcBlkIoDev, DevPos, 1) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  //
  // for drive A: 1CH, drive B: 2DH
  //
  Data = 0x0C;
  Data = (UINT8) (Data | (SELECT_DRV & DevPos));
  if (DevPos == 0) {
    Data |= DRVA_MOTOR_ON;
  } else {
    Data |= DRVB_MOTOR_ON;
  }

  Info->MotorOn = FALSE;

  //
  // Turn on the motor and wait for some time to ensure it takes effect.
  //
  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DOR), Data);
  MicroSecondDelay (FDC_LONG_DELAY);

  Info->MotorOn = TRUE;

  return EFI_SUCCESS;
}

/**
  Turn off the motor of floppy drive.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Info             Information of floppy device.

**/
VOID
MotorOff (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
{
  UINT8 Data;
  UINT8 DevPos;

  DevPos = Info->DevPos;

  if (!Info->MotorOn) {
    return;
  }
  //
  // The motor is on, so need motor off
  //
  Data = 0x0C;
  Data = (UINT8) (Data | (SELECT_DRV & DevPos));

  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DOR), Data);
  MicroSecondDelay (FDC_SHORT_DELAY);

  Info->MotorOn = FALSE;
}

/**
  Recalibrate the FDC device.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Info             Information of floppy device.

  @retval EFI_SUCCESS      FDC successfully recalibrated.
  @retval EFI_DEVICE_ERROR Fail to send RECALIBRATE_CMD.
  @retval EFI_DEVICE_ERROR Fail to get status value of Sts0 and Pcn.
  @retval EFI_DEVICE_ERROR Fail to recalibrate FDC device.

**/
EFI_STATUS
Recalibrate (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
{
  FDC_COMMAND_PACKET2 Command;
  UINTN               Index;
  UINT8               Sts0;
  UINT8               Pcn;
  UINT8               *Pointer;
  UINT8               Count;
  UINT8               DevPos;

  DevPos  = Info->DevPos;

  //
  // We would try twice.
  //
  Count   = 2;
  while (Count > 0) {
    ZeroMem (&Command, sizeof (FDC_COMMAND_PACKET2));
    Command.CommandCode = RECALIBRATE_CMD;
    //
    // drive select
    //
    if (DevPos == 0) {
      Command.DiskHeadSel = 0;
    } else {
      Command.DiskHeadSel = 1;
    }

    Pointer = (UINT8 *) (&Command);
    for (Index = 0; Index < sizeof (FDC_COMMAND_PACKET2); Index++) {
      if (DataOutByte (FdcBlkIoDev, Pointer++) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
    }

    MicroSecondDelay (FDC_RECALIBRATE_DELAY);

    if (SenseIntStatus (FdcBlkIoDev, &Sts0, &Pcn) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }

    if ((Sts0 & 0xf0) == BIT5 && Pcn == 0) {
      //
      // Recalibration is successful. 
      //
      Info->Pcn = 0;
      Info->NeedRecalibrate = FALSE;

      return EFI_SUCCESS;
    } else {
      //
      // Recalibration is not successful. Try again.
      // If trial is used out, return EFI_DEVICE_ERROR.
      //
      Count--;
      if (Count == 0) {
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Seek for the cylinder according to given LBA.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Info             Information of floppy device.
  @param  Lba              LBA for which to seek for cylinder.

  @retval EFI_SUCCESS      Successfully moved to the destination cylinder.
  @retval EFI_SUCCESS      Destination cylinder is just the present cylinder.
  @retval EFI_DEVICE_ERROR Fail to move to the destination cylinder.

**/
EFI_STATUS
Seek (
  IN     FDC_BLK_IO_DEV         *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN     EFI_PEI_LBA            Lba
  )
{
  FDC_SEEK_CMD      Command;
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             Head;
  UINT8             Cylinder;
  UINT8             Sts0;
  UINT8             *Pointer;
  UINT8             Pcn;
  UINTN             Index;
  UINT8             Gap;
  UINT8             DevPos;

  DevPos = Info->DevPos;
  if (Info->NeedRecalibrate) {
    if (Recalibrate (FdcBlkIoDev, Info) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Recalibrate Success
    //
    Info->NeedRecalibrate = FALSE;
  }

  //
  // Get the base of disk parameter information corresponding to its type.
  //
  Para        = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);
  EndOfTrack  = Para->EndOfTrack;
  //
  // Calculate cylinder based on Lba and EOT
  //
  Cylinder = (UINT8) ((UINTN) Lba / EndOfTrack / 2);

  //
  // If the dest cylinder is the present cylinder, unnecessary to do the seek operation
  //
  if (Info->Pcn == Cylinder) {
    return EFI_SUCCESS;
  }

  //
  // Calculate the head : 0 or 1
  //
  Head = (UINT8) ((UINTN) Lba / EndOfTrack % 2);

  ZeroMem (&Command, sizeof (FDC_SEEK_CMD));
  Command.CommandCode = SEEK_CMD;
  if (DevPos == 0) {
    Command.DiskHeadSel = 0;
  } else {
    Command.DiskHeadSel = 1;
  }

  //
  // Send command to move to destination cylinder.
  //
  Command.DiskHeadSel = (UINT8) (Command.DiskHeadSel | (Head << 2));
  Command.NewCylinder = Cylinder;

  Pointer = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDC_SEEK_CMD); Index++) {
    if (DataOutByte (FdcBlkIoDev, Pointer++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  MicroSecondDelay (FDC_SHORT_DELAY);

  //
  // Calculate waiting time, which is proportional to the gap between destination
  // cylinder and present cylinder.
  //
  if (Info->Pcn > Cylinder) {
    Gap = (UINT8) (Info->Pcn - Cylinder);
  } else {
    Gap = (UINT8) (Cylinder - Info->Pcn);
  }

  MicroSecondDelay ((Gap + 1) * FDC_LONG_DELAY);

  //
  // Confirm if the new cylinder is the destination and status is correct.
  //
  if (SenseIntStatus (FdcBlkIoDev, &Sts0, &Pcn) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((Sts0 & 0xf0) == BIT5) {
    Info->Pcn             = Command.NewCylinder;
    Info->NeedRecalibrate = FALSE;
    return EFI_SUCCESS;
  } else {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
}

/**
  Check if diskette is changed.

  @param  FdcBlkIoDev       Instance of FDC_BLK_IO_DEV
  @param  Info              Information of floppy device.

  @retval EFI_SUCCESS       Diskette is not changed.
  @retval EFI_MEDIA_CHANGED Diskette is changed.
  @retval EFI_NO_MEDIA      No diskette.
  @retval EFI_DEVICE_ERROR  Fail to do the seek or recalibrate operation.

**/
EFI_STATUS
DisketChanged (
  IN FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info
  )
{
  EFI_STATUS  Status;
  UINT8       Data;

  //
  // Check change line
  //
  Data = IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DIR));

  MicroSecondDelay (FDC_SHORT_DELAY);

  if ((Data & DIR_DCL) == DIR_DCL) {
    if (Info->Pcn != 0) {
      Status = Recalibrate (FdcBlkIoDev, Info);
    } else {
      Status = Seek (FdcBlkIoDev, Info, 0x30);
    }

    if (Status != EFI_SUCCESS) {
      //
      // Fail to do the seek or recalibrate operation
      //
      return EFI_DEVICE_ERROR;
    }

    Data = IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_DIR));

    MicroSecondDelay (FDC_SHORT_DELAY);

    if ((Data & DIR_DCL) == DIR_DCL) {
      return EFI_NO_MEDIA;
    }

    return EFI_MEDIA_CHANGED;
  }

  return EFI_SUCCESS;
}

/**
  Detects if FDC device exists.

  @param  FdcBlkIoDev  Instance of FDC_BLK_IO_DEV
  @param  Info         Information of floppy device.
  @param  MediaInfo    Information of floppy media.

  @retval TRUE         FDC device exists and is working properly.
  @retval FALSE        FDC device does not exist or cannot work properly.

**/
BOOLEAN
DiscoverFdcDevice (
  IN  FDC_BLK_IO_DEV             *FdcBlkIoDev,
  IN  OUT PEI_FLOPPY_DEVICE_INFO *Info,
  OUT EFI_PEI_BLOCK_IO_MEDIA     *MediaInfo
  )
{
  EFI_STATUS        Status;
  DISKET_PARA_TABLE *Para;

  Status = MotorOn (FdcBlkIoDev, Info);
  if (Status != EFI_SUCCESS) {
    return FALSE;
  }

  Status = Recalibrate (FdcBlkIoDev, Info);

  if (Status != EFI_SUCCESS) {
    MotorOff (FdcBlkIoDev, Info);
    return FALSE;
  }
  //
  // Set Media Parameter
  //
  MediaInfo->DeviceType   = LegacyFloppy;
  MediaInfo->MediaPresent = TRUE;

  //
  // Check Media
  //
  Status = DisketChanged (FdcBlkIoDev, Info);
  if (Status == EFI_NO_MEDIA) {
    //
    // No diskette in floppy.
    //
    MediaInfo->MediaPresent = FALSE;    
  } else if (Status != EFI_MEDIA_CHANGED && Status != EFI_SUCCESS) {
    //
    // EFI_DEVICE_ERROR
    //
    MotorOff (FdcBlkIoDev, Info);
    return FALSE;
  }

  MotorOff (FdcBlkIoDev, Info);

  //
  // Get the base of disk parameter information corresponding to its type.
  //
  Para                  = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);

  MediaInfo->BlockSize  = BytePerSector[Para->Number];
  MediaInfo->LastBlock  = Para->EndOfTrack * 2 * (Para->MaxTrackNum + 1) - 1;

  return TRUE;
}

/**
  Enumerate floppy device

  @param  FdcBlkIoDev  Instance of floppy device controller

  @return Number of FDC devices.

**/
UINT8
FdcEnumeration (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev
  )
{
  UINT8                   DevPos;
  UINT8                   DevNo;
  EFI_PEI_BLOCK_IO_MEDIA  MediaInfo;
  EFI_STATUS              Status;

  DevNo = 0;

  //
  // DevPos=0 means Drive A, 1 means Drive B.
  //
  for (DevPos = 0; DevPos < 2; DevPos++) {
    //
    // Detecting device presence
    //
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_PC_PRESENCE_DETECT);

    //
    // Reset FDC
    //
    Status = FdcReset (FdcBlkIoDev, DevPos);

    if (EFI_ERROR (Status)) {
      continue;
    }

    FdcBlkIoDev->DeviceInfo[DevPos].DevPos          = DevPos;
    FdcBlkIoDev->DeviceInfo[DevPos].Pcn             = 0;
    FdcBlkIoDev->DeviceInfo[DevPos].MotorOn         = FALSE;
    FdcBlkIoDev->DeviceInfo[DevPos].NeedRecalibrate = TRUE;
    FdcBlkIoDev->DeviceInfo[DevPos].Type            = FdcType1440K1440K;

    //
    // Discover FDC device
    //
    if (DiscoverFdcDevice (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DevPos]), &MediaInfo)) {
      FdcBlkIoDev->DeviceInfo[DevNo].DevPos           = DevPos;

      FdcBlkIoDev->DeviceInfo[DevNo].Pcn              = FdcBlkIoDev->DeviceInfo[DevPos].Pcn;
      FdcBlkIoDev->DeviceInfo[DevNo].MotorOn          = FdcBlkIoDev->DeviceInfo[DevPos].MotorOn;
      FdcBlkIoDev->DeviceInfo[DevNo].NeedRecalibrate  = FdcBlkIoDev->DeviceInfo[DevPos].NeedRecalibrate;
      FdcBlkIoDev->DeviceInfo[DevNo].Type             = FdcBlkIoDev->DeviceInfo[DevPos].Type;

      CopyMem (
        &(FdcBlkIoDev->DeviceInfo[DevNo].MediaInfo),
        &MediaInfo,
        sizeof (EFI_PEI_BLOCK_IO_MEDIA)
        );

      DevNo++;
    } else {
      //
      // Assume controller error
      //
      REPORT_STATUS_CODE (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_EC_CONTROLLER_ERROR
        );
    }
  }

  FdcBlkIoDev->DeviceCount = DevNo;
  return DevNo;
}

/**
  Checks result reflected by FDC_RESULT_PACKET.

  @param  Result           FDC_RESULT_PACKET read from FDC after certain operation.
  @param  Info             Information of floppy device.

  @retval EFI_SUCCESS      Result is healthy.
  @retval EFI_DEVICE_ERROR Result is not healthy.

**/
EFI_STATUS
CheckResult (
  IN  FDC_RESULT_PACKET         *Result,
  OUT PEI_FLOPPY_DEVICE_INFO    *Info
  )
{
  if ((Result->Status0 & STS0_IC) != IC_NT) {
    if ((Result->Status0 & STS0_SE) == BIT5) {
      //
      // Seek error
      //
      Info->NeedRecalibrate = TRUE;
    }

    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register1
  //
  if ((Result->Status1 & (STS1_EN | STS1_DE | STS1_OR | STS1_ND | STS1_NW | STS1_MA)) != 0) {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }
  //
  // Check Status Register2
  //
  if ((Result->Status2 & (STS2_CM | STS2_DD | STS2_WC | STS2_BC | STS2_MD)) != 0) {
    Info->NeedRecalibrate = TRUE;
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Fill parameters for command packet.

  @param  Info    Information of floppy device.
  @param  Lba     Logical block address.
  @param  Command Command for which for fill parameters.

**/
VOID
FillPara (
  IN  PEI_FLOPPY_DEVICE_INFO *Info,
  IN  EFI_PEI_LBA            Lba,
  OUT FDC_COMMAND_PACKET1    *Command
  )
{
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             DevPos;

  DevPos      = Info->DevPos;

  //
  // Get the base of disk parameter information corresponding to its type.
  //
  Para        = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);

  EndOfTrack  = Para->EndOfTrack;

  if (DevPos == 0) {
    Command->DiskHeadSel = 0;
  } else {
    Command->DiskHeadSel = 1;
  }

  Command->Cylinder    = (UINT8) ((UINTN) Lba / EndOfTrack / 2);
  Command->Head        = (UINT8) ((UINTN) Lba / EndOfTrack % 2);
  Command->Sector      = (UINT8) ((UINT8) ((UINTN) Lba % EndOfTrack) + 1);
  Command->DiskHeadSel = (UINT8) (Command->DiskHeadSel | (Command->Head << 2));
  Command->Number      = Para->Number;
  Command->EndOfTrack  = Para->EndOfTrack;
  Command->GapLength   = Para->GapLength;
  Command->DataLength  = Para->DataLength;
}

/**
  Setup specifed FDC device.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  DevPos           Index of FDC device.

  @retval EFI_SUCCESS      FDC device successfully set up.
  @retval EFI_DEVICE_ERROR FDC device has errors.

**/
EFI_STATUS
Setup (
  IN  FDC_BLK_IO_DEV  *FdcBlkIoDev,
  IN  UINT8           DevPos
  )
{
  EFI_STATUS  Status;

  IoWrite8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_CCR), 0x0);

  MicroSecondDelay (FDC_MEDIUM_DELAY);

  Status = Specify (FdcBlkIoDev);
  return Status;
}

/**
  Setup DMA channels to read data.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Buffer           Memory buffer for DMA transfer.
  @param  BlockSize        the number of the bytes in one block.
  @param  NumberOfBlocks   Number of blocks to read.

**/
VOID
SetDMA (
  IN FDC_BLK_IO_DEV   *FdcBlkIoDev,
  IN VOID             *Buffer,
  IN UINTN            BlockSize,
  IN UINTN            NumberOfBlocks
  )
{
  UINT8 Data;
  UINTN Count;

  //
  // Mask DMA channel 2;
  //
  IoWrite8 (R_8237_DMA_WRSMSK_CH0_3, B_8237_DMA_WRSMSK_CMS | 2);

  //
  // Clear first/last flip flop
  //
  IoWrite8 (R_8237_DMA_CBPR_CH0_3, B_8237_DMA_WRSMSK_CMS | 2);

  //
  // Set mode
  //
  IoWrite8 (R_8237_DMA_CHMODE_CH0_3, V_8237_DMA_CHMODE_SINGLE | V_8237_DMA_CHMODE_IO2MEM | 2);

  //
  // Set base address and page register
  //
  Data = (UINT8) (UINTN) Buffer;
  IoWrite8 (R_8237_DMA_BASE_CA_CH2, Data);
  Data = (UINT8) ((UINTN) Buffer >> 8);
  IoWrite8 (R_8237_DMA_BASE_CA_CH2, Data);

  Data = (UINT8) ((UINTN) Buffer >> 16);
  IoWrite8 (R_8237_DMA_MEM_LP_CH2, Data);

  //
  // Set count register
  //
  Count = BlockSize * NumberOfBlocks - 1;
  Data  = (UINT8) (Count & 0xff);
  IoWrite8 (R_8237_DMA_BASE_CC_CH2, Data);
  Data = (UINT8) (Count >> 8);
  IoWrite8 (R_8237_DMA_BASE_CC_CH2, Data);

  //
  // Clear channel 2 mask
  //
  IoWrite8 (R_8237_DMA_WRSMSK_CH0_3, 0x02);
}


/**
  According to the block range specified by Lba and NumberOfBlocks, calculate
  the number of blocks in the same sector, which can be transferred in a batch.

  @param  Info           Information of floppy device.
  @param  Lba            Start address of block range.
  @param  NumberOfBlocks Number of blocks of the range.

  @return Number of blocks in the same sector.

**/
UINTN
GetTransferBlockCount (
  IN  PEI_FLOPPY_DEVICE_INFO *Info,
  IN  EFI_PEI_LBA            Lba,
  IN  UINTN                  NumberOfBlocks
  )
{
  DISKET_PARA_TABLE *Para;
  UINT8             EndOfTrack;
  UINT8             Head;
  UINT8             SectorsInTrack;

  //
  // Get the base of disk parameter information corresponding to its type.
  //
  Para            = (DISKET_PARA_TABLE *) ((UINT8 *) DiskPara + sizeof (DISKET_PARA_TABLE) * Info->Type);

  EndOfTrack      = Para->EndOfTrack;
  Head            = (UINT8) ((UINTN) Lba / EndOfTrack % 2);

  SectorsInTrack  = (UINT8) (EndOfTrack * (2 - Head) - (UINT8) ((UINTN) Lba % EndOfTrack));
  if (SectorsInTrack < NumberOfBlocks) {
    //
    // Not all the block range locates in the same sector
    //
    return SectorsInTrack;
  } else {
    //
    // All the block range is in the same sector.
    //
    return NumberOfBlocks;
  }
}

/**
  Read data sector from FDC device.

  @param  FdcBlkIoDev      Instance of FDC_BLK_IO_DEV.
  @param  Info             Information of floppy device.
  @param  Buffer           Buffer to setup for DMA.
  @param  Lba              The start address to read.
  @param  NumberOfBlocks   Number of blocks to read.

  @retval EFI_SUCCESS      Data successfully read out.
  @retval EFI_DEVICE_ERROR FDC device has errors.
  @retval EFI_TIMEOUT      Command does not take effect in time.

**/
EFI_STATUS
ReadDataSector (
  IN     FDC_BLK_IO_DEV         *FdcBlkIoDev,
  IN OUT PEI_FLOPPY_DEVICE_INFO *Info,
  IN     VOID                   *Buffer,
  IN     EFI_PEI_LBA            Lba,
  IN     UINTN                  NumberOfBlocks
  )
{
  EFI_STATUS          Status;
  FDC_COMMAND_PACKET1 Command;
  FDC_RESULT_PACKET   Result;
  UINTN               Index;
  UINTN               Times;
  UINT8               *Pointer;

  Status = Seek (FdcBlkIoDev, Info, Lba);
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Set up DMA
  //
  SetDMA (FdcBlkIoDev, Buffer, Info->MediaInfo.BlockSize, NumberOfBlocks);

  //
  // Allocate Read command packet
  //
  ZeroMem (&Command, sizeof (FDC_COMMAND_PACKET1));
  Command.CommandCode = READ_DATA_CMD | CMD_MT | CMD_MFM | CMD_SK;

  //
  // Fill parameters for command.
  //
  FillPara (Info, Lba, &Command);

  //
  // Write command bytes to FDC
  //
  Pointer = (UINT8 *) (&Command);
  for (Index = 0; Index < sizeof (FDC_COMMAND_PACKET1); Index++) {
    if (DataOutByte (FdcBlkIoDev, Pointer++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Wait for some time until command takes effect.
  //
  Times = (STALL_1_SECOND / FDC_CHECK_INTERVAL) + 1;
  do {
    if ((IoRead8 ((UINT16) (PcdGet16 (PcdFdcBaseAddress) + FDC_REGISTER_MSR)) & 0xc0) == 0xc0) {
      break;
    }

    MicroSecondDelay (FDC_SHORT_DELAY);
  } while (--Times > 0);

  if (Times == 0) {
    //
    // Command fails to take effect in time, return EFI_TIMEOUT.
    //
    return EFI_TIMEOUT;
  }

  //
  // Read result bytes from FDC
  //
  Pointer = (UINT8 *) (&Result);
  for (Index = 0; Index < sizeof (FDC_RESULT_PACKET); Index++) {
    if (DataInByte (FdcBlkIoDev, Pointer++) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  return CheckResult (&Result, Info);
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
FdcGetNumberOfBlockDevices (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  OUT  UINTN                             *NumberBlockDevices
  )
{
  FDC_BLK_IO_DEV  *FdcBlkIoDev;

  FdcBlkIoDev = NULL;

  FdcBlkIoDev         = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  *NumberBlockDevices = FdcBlkIoDev->DeviceCount;

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
  
  @retval EFI_SUCCESS        Media information about the specified block device 
                             was obtained successfully.
  @retval EFI_DEVICE_ERROR   Cannot get the media information due to a hardware 
                             error.
  @retval Others             Other failure occurs.

**/
EFI_STATUS
EFIAPI
FdcGetBlockDeviceMediaInfo (
  IN   EFI_PEI_SERVICES                     **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI        *This,
  IN   UINTN                                DeviceIndex,
  OUT  EFI_PEI_BLOCK_IO_MEDIA               *MediaInfo
  )
{
  UINTN           DeviceCount;
  FDC_BLK_IO_DEV  *FdcBlkIoDev;
  BOOLEAN         Healthy;
  UINTN           Index;

  FdcBlkIoDev = NULL;

  if (This == NULL || MediaInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FdcBlkIoDev = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  DeviceCount = FdcBlkIoDev->DeviceCount;

  //
  // DeviceIndex is a value from 1 to NumberBlockDevices.
  //
  if ((DeviceIndex < 1) || (DeviceIndex > DeviceCount) || (DeviceIndex > 2)) {
    return EFI_INVALID_PARAMETER;
  }

  Index = DeviceIndex - 1;
  //
  // Probe media and retrieve latest media information
  //
  Healthy = DiscoverFdcDevice (
              FdcBlkIoDev,
              &FdcBlkIoDev->DeviceInfo[Index],
              MediaInfo
              );

  if (!Healthy) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (
    &(FdcBlkIoDev->DeviceInfo[Index].MediaInfo),
    MediaInfo,
    sizeof (EFI_PEI_BLOCK_IO_MEDIA)
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
FdcReadBlocks (
  IN   EFI_PEI_SERVICES                  **PeiServices,
  IN   EFI_PEI_RECOVERY_BLOCK_IO_PPI     *This,
  IN   UINTN                             DeviceIndex,
  IN   EFI_PEI_LBA                       StartLBA,
  IN   UINTN                             BufferSize,
  OUT  VOID                              *Buffer
  )
{
  EFI_PEI_BLOCK_IO_MEDIA MediaInfo;
  EFI_STATUS            Status;
  UINTN                 Count;
  UINTN                 NumberOfBlocks;
  UINTN                 BlockSize;
  FDC_BLK_IO_DEV        *FdcBlkIoDev;
  VOID                  *MemPage;

  FdcBlkIoDev = NULL;
  ZeroMem (&MediaInfo, sizeof (EFI_PEI_BLOCK_IO_MEDIA));

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FdcBlkIoDev = PEI_RECOVERY_FDC_FROM_BLKIO_THIS (This);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FdcGetBlockDeviceMediaInfo (PeiServices, This, DeviceIndex, &MediaInfo);
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (!MediaInfo.MediaPresent) {
    return EFI_NO_MEDIA;
  }

  BlockSize = MediaInfo.BlockSize;

  //
  // If BufferSize cannot be divided by block size of FDC device,
  // return EFI_BAD_BUFFER_SIZE.
  //
  if (BufferSize % BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;

  if ((StartLBA + NumberOfBlocks - 1) > FdcBlkIoDev->DeviceInfo[DeviceIndex - 1].MediaInfo.LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  MemPage = AllocatePages (EFI_SIZE_TO_PAGES (BufferSize));
  if ((MemPage == NULL) || ((UINTN) MemPage >= ISA_MAX_MEMORY_ADDRESS)) {
    //
    // If fail to allocate memory under ISA_MAX_MEMORY_ADDRESS, designate the address space for DMA
    //
    MemPage = (VOID *) ((UINTN) (UINT32) 0x0f00000);
  }
  Status = MotorOn (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex - 1]));
  if (Status != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Status = Setup (FdcBlkIoDev, FdcBlkIoDev->DeviceInfo[DeviceIndex - 1].DevPos);
  if (Status != EFI_SUCCESS) {
    MotorOff (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex - 1]));
    return EFI_DEVICE_ERROR;
  }
  //
  // Read data in batches.
  // Blocks in the same cylinder are read out in a batch.
  //
  while ((Count = GetTransferBlockCount (
                    &(FdcBlkIoDev->DeviceInfo[DeviceIndex - 1]),
                    StartLBA,
                    NumberOfBlocks
                    )) != 0 && Status == EFI_SUCCESS) {
    Status = ReadDataSector (
               FdcBlkIoDev,
               &(FdcBlkIoDev->DeviceInfo[DeviceIndex - 1]),
               MemPage,
               StartLBA,
               Count
               );
    CopyMem (Buffer, MemPage, BlockSize * Count);
    StartLBA += Count;
    NumberOfBlocks -= Count;
    Buffer = (VOID *) ((UINTN) Buffer + Count * BlockSize);
  }

  MotorOff (FdcBlkIoDev, &(FdcBlkIoDev->DeviceInfo[DeviceIndex - 1]));

  switch (Status) {
  case EFI_SUCCESS:
    return EFI_SUCCESS;

  default:
    FdcReset (FdcBlkIoDev, FdcBlkIoDev->DeviceInfo[DeviceIndex - 1].DevPos);
    return EFI_DEVICE_ERROR;
  }
}

/**
  Initializes the floppy disk controller and installs FDC Block I/O PPI.

  @param  FileHandle            Handle of the file being invoked.
  @param  PeiServices           Describes the list of possible PEI Services.

  @retval EFI_SUCCESS           Successfully initialized FDC and installed PPI.
  @retval EFI_NOT_FOUND         Cannot find FDC device.
  @retval EFI_OUT_OF_RESOURCES  Have no enough memory to create instance or descriptors.
  @retval Other                 Fail to install FDC Block I/O PPI.

**/
EFI_STATUS
EFIAPI
FdcPeimEntry (
  IN  EFI_PEI_FILE_HANDLE         FileHandle,
  IN  CONST EFI_PEI_SERVICES      **PeiServices
  )
{
  EFI_STATUS            Status;
  FDC_BLK_IO_DEV        *FdcBlkIoDev;
  UINTN                 DeviceCount;
  UINT32                Index;

  Status = PeiServicesRegisterForShadow (FileHandle);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate memory for instance of FDC_BLK_IO_DEV and copy initial value
  // from template to it. 
  //
  FdcBlkIoDev = AllocatePages (EFI_SIZE_TO_PAGES(sizeof (FDC_BLK_IO_DEV)));
  if (FdcBlkIoDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (FdcBlkIoDev, &mBlockIoDevTemplate, sizeof (mBlockIoDevTemplate));

  //
  // Initialize DMA controller to enable all channels.
  //
  for (Index = 0; Index < sizeof (mRegisterTable) / sizeof (PEI_DMA_TABLE); Index++) {
    IoWrite8 (mRegisterTable[Index].Register, mRegisterTable[Index].Value);
  }
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, EFI_PERIPHERAL_REMOVABLE_MEDIA + EFI_P_PC_INIT);

  //
  // Enumerate FDC devices.
  //
  DeviceCount = FdcEnumeration (FdcBlkIoDev);
  if (DeviceCount == 0) {
    return EFI_NOT_FOUND;
  }

  FdcBlkIoDev->PpiDescriptor.Ppi = &FdcBlkIoDev->FdcBlkIo;

  return PeiServicesInstallPpi (&FdcBlkIoDev->PpiDescriptor);
}
